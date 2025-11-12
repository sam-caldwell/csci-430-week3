#include "CodeGen.h"

// Suppress deprecation warnings from LLVM headers under C++23
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

#include <cassert>
#include <stdexcept>
#include <sstream>

namespace fakelang {

/// Initialize an empty module and IRBuilder bound to our LLVMContext.
CodeGen::CodeGen() {
  module_ = std::make_unique<llvm::Module>("fakelang-module", ctx_);
  builder_ = std::make_unique<llvm::IRBuilder<>>(ctx_);
}

void CodeGen::setSource(std::string sourceText, std::string filename) {
  sourceFilename_ = std::move(filename);
  sourceText_ = std::move(sourceText);
  // Split into lines for quick lookup; keep 1-based mapping via index+1
  sourceLines_.clear();
  std::string cur;
  std::istringstream iss(sourceText_);
  while (std::getline(iss, cur)) {
    // Drop trailing carriage returns for Windows-style newlines
    if (!cur.empty() && cur.back() == '\r') cur.pop_back();
    sourceLines_.push_back(cur);
  }
}

llvm::Type* CodeGen::tyVoid() { return llvm::Type::getVoidTy(ctx_); }
llvm::Type* CodeGen::tyI32() { return llvm::Type::getInt32Ty(ctx_); }
llvm::Type* CodeGen::tyI8() { return llvm::Type::getInt8Ty(ctx_); }
llvm::PointerType* CodeGen::tyI8Ptr() {
  return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(ctx_));
}

/// Map fakelang type names to canonical LLVM types for returns.
llvm::Type* CodeGen::retTypeFor(const std::string& typeName) {
  if (typeName == "Int") return tyI32();
  if (typeName == "String") return tyI8Ptr();
  // Classes: for simplicity, methods return only String in this demo
  return tyI8Ptr();
}

/// Construct the LLVM function type for a method on `classTy` returning
/// `retTypeName`. Methods take a single implicit `this` pointer parameter.
llvm::FunctionType* CodeGen::methodFnTy(const std::string& retTypeName,
                                        llvm::StructType* classTy) {
  // Signature: ret (ptr)
  std::vector<llvm::Type*> params{llvm::PointerType::getUnqual(classTy)};
  return llvm::FunctionType::get(retTypeFor(retTypeName), params, /*isVarArg=*/false);
}

/// Entry point: lower AST to LLVM IR and verify module correctness.
void CodeGen::generate(const Program& program, const std::string& moduleName) {
  module_->setModuleIdentifier(moduleName);
  classes_.clear();

  computeClassLayouts(program);
  declareTypes();
  declareAndDefineMethods();
  defineVTables();
  defineFunctions(program);

  // Validate the module for sanity
  std::string err;
  llvm::raw_string_ostream os(err);
  if (llvm::verifyModule(*module_, &os)) {
    os.flush();
    throw std::runtime_error("Invalid LLVM module generated: " + err);
  }
}

// Attach a simple metadata string to an instruction capturing source info.
void CodeGen::annotate(llvm::Value* v, const SourceRange& rng, std::string_view kind) {
  if (!v) return;
  if (auto* I = llvm::dyn_cast<llvm::Instruction>(v)) {
    std::string msg;
    llvm::raw_string_ostream ss(msg);
    ss << sourceFilename_ << ":" << rng.start.line << ":" << rng.start.column
       << "-" << rng.end.line << ":" << rng.end.column;
    if (!kind.empty()) ss << " | " << kind;
    auto snippet = srcSnippet(rng);
    if (!snippet.empty()) ss << " | " << snippet;
    ss.flush();
    auto* s = llvm::MDString::get(ctx_, msg);
    auto* md = llvm::MDNode::get(ctx_, s);
    I->setMetadata("fakelang.src", md);
  }
}

std::string CodeGen::srcSnippet(const SourceRange& rng) const {
  // Return a single-line snippet (first line) trimmed to 80 chars
  if (rng.start.line <= 0 || static_cast<size_t>(rng.start.line) > sourceLines_.size()) return {};
  const std::string& line = sourceLines_[static_cast<size_t>(rng.start.line) - 1];
  // Columns are 1-based; clamp
  size_t startCol = rng.start.column > 0 ? static_cast<size_t>(rng.start.column - 1) : 0;
  if (startCol >= line.size()) return line;
  std::string s = line.substr(startCol);
  // Trim
  if (s.size() > 80) s.resize(80);
  // Replace tabs with spaces for readability
  for (auto& c : s) if (c == '\t') c = ' ';
  return s;
}

/// Compute vtable slot layouts for all classes, honoring inheritance and
/// override/virtual markers.
void CodeGen::computeClassLayouts(const Program& p) {
  // First, record classes and bases
  for (const auto& c : p.classes) {
    ClassInfo info;
    info.ast = &c;
    info.name = c.name;
    info.base = c.baseName;
    classes_.emplace(info.name, std::move(info));
  }

  // Compute vtable method layout with a simple DFS by declaration order.
  // Assumes base classes appear before derived classes for this demo.
  for (auto& [name, info] : classes_) {
    ClassLayout layout;
    if (info.base) {
      auto it = classes_.find(*info.base);
      if (it == classes_.end()) throw std::runtime_error("Unknown base class: " + *info.base);
      layout = it->second.layout; // copy base layout
    }
    // Process methods
    for (const auto& m : info.ast->methods) {
      if (m.attr == MethodAttr::Override) {
        auto it = layout.slotOf.find(m.name);
        if (it == layout.slotOf.end()) {
          throw std::runtime_error("Method '" + m.name + "' marked override but no base method");
        }
        // slot remains the same; implementation will be replaced
      } else if (m.attr == MethodAttr::Virtual) {
        layout.slotOf[m.name] = layout.methods.size();
        layout.methods.push_back(m.name);
      } else {
        // Non-virtual methods are ignored for vtable purposes in this demo
      }
    }
    info.layout = std::move(layout);
  }
}

/// Declare opaque struct types for each class and its vtable, and set their
/// bodies (field types) once layouts are known.
void CodeGen::declareTypes() {
  // Create struct types for classes and vtables
  for (auto& [name, info] : classes_) {
    info.vtableTy = llvm::StructType::create(ctx_, "vtable." + name);
    info.classTy = llvm::StructType::create(ctx_, "class." + name);
  }
  for (auto& [name, info] : classes_) {
    // vtable body: N x i8*
    std::vector<llvm::Type*> vtElems(info.layout.methods.size(), tyI8Ptr());
    info.vtableTy->setBody(vtElems, /*isPacked=*/false);
    // class body: { ptr to vtable }
    std::vector<llvm::Type*> clsElems{llvm::PointerType::getUnqual(info.vtableTy)};
    info.classTy->setBody(clsElems, /*isPacked=*/false);
  }
}

/// Declare and define LLVM functions for each class method, emitting bodies
/// by lowering statements. Methods have no parameters in this demo.
void CodeGen::declareAndDefineMethods() {
  // Create functions for each method
  for (auto& [name, info] : classes_) {
    for (const auto& m : info.ast->methods) {
      auto* fty = methodFnTy(m.returnType.name, info.classTy);
      auto* fn = llvm::Function::Create(fty, llvm::GlobalValue::ExternalLinkage,
                                        info.name + "." + m.name, module_.get());
      info.methods[m.name] = fn;

      // Define body
      auto* entry = llvm::BasicBlock::Create(ctx_, "entry", fn);
      builder_->SetInsertPoint(entry);

      std::map<std::string, ScopeVar> scope; // no locals in methods in demo
      // currentClassTy is used only for potential field access (not present)
      for (const auto& s : m.body) {
        codegenStmt(s.get(), scope, m.returnType.name, info.classTy);
        if (builder_->GetInsertBlock()->getTerminator()) break;
      }
      // If control reaches here without an explicit return, insert default return
      if (!builder_->GetInsertBlock()->getTerminator()) {
        if (m.returnType.name == "Int") {
          builder_->CreateRet(llvm::ConstantInt::get(tyI32(), 0));
        } else {
          builder_->CreateRet(llvm::UndefValue::get(retTypeFor(m.returnType.name)));
        }
      }
    }
  }
}

/// Define and initialize vtable globals for each class using the previously
/// computed layouts and method implementations.
void CodeGen::defineVTables() {
  for (auto& [name, info] : classes_) {
    // Build initializer elements per slot
    std::vector<llvm::Constant*> elems;
    elems.reserve(info.layout.methods.size());
    for (size_t i = 0; i < info.layout.methods.size(); ++i) {
      const std::string& mname = info.layout.methods[i];
      llvm::Function* impl = implementationFor(name, mname);
      auto* c = llvm::ConstantExpr::getPointerCast(impl, tyI8Ptr());
      elems.push_back(c);
    }
    llvm::Constant* init = nullptr;
    if (elems.empty()) {
      init = llvm::UndefValue::get(info.vtableTy);
    } else {
      init = llvm::ConstantStruct::get(info.vtableTy, elems);
    }
    info.vtableGlobal = new llvm::GlobalVariable(
        *module_, info.vtableTy, /*isConstant=*/true,
        llvm::GlobalValue::PrivateLinkage, init, "vtable." + name);
  }
}

/// Define free-standing functions like `main`, and declare `puts`.
void CodeGen::defineFunctions(const Program& p) {
  // External: declare puts(ptr)
  (void)getOrDeclarePuts();

  // Free functions: only 'main' is needed for the demo
  for (const auto& f : p.functions) {
    auto* fty = llvm::FunctionType::get(retTypeFor(f.returnType.name), /*params*/{}, false);
    auto* fn = llvm::Function::Create(fty, llvm::GlobalValue::ExternalLinkage, f.name, module_.get());
    auto* entry = llvm::BasicBlock::Create(ctx_, "entry", fn);
    builder_->SetInsertPoint(entry);

    std::map<std::string, ScopeVar> scope;
    // Codegen body statements
    for (const auto& s : f.body) {
      codegenStmt(s.get(), scope, f.returnType.name, /*currentClassTy*/ nullptr);
      if (builder_->GetInsertBlock()->getTerminator()) break;
    }
    // If no explicit return
    if (!builder_->GetInsertBlock()->getTerminator()) {
      if (f.returnType.name == "Int") builder_->CreateRet(llvm::ConstantInt::get(tyI32(), 0));
      else builder_->CreateRet(llvm::UndefValue::get(retTypeFor(f.returnType.name)));
    }
  }
}

/// Lazily declare libc `puts(char const*)` and return the function.
llvm::Function* CodeGen::getOrDeclarePuts() {
  if (auto* f = module_->getFunction("puts")) return f;
  auto* fty = llvm::FunctionType::get(tyI32(), {tyI8Ptr()}, false);
  return llvm::Function::Create(fty, llvm::GlobalValue::ExternalLinkage, "puts", module_.get());
}

/// Lower an expression in the current function/method context and return the
/// resulting LLVM value. The optional `expectedType` is used to guide codegen
/// (e.g., for method-call return types).
llvm::Value* CodeGen::codegenExpr(const Expr* e,
                                  std::map<std::string, ScopeVar>& scope,
                                  const std::string& expectedType) {
  if (auto* se = dynamic_cast<const StringExpr*>(e)) {
    // Global string emission does not create an instruction to annotate
    return builder_->CreateGlobalStringPtr(se->value);
  }
  if (auto* ie = dynamic_cast<const IntExpr*>(e)) {
    return llvm::ConstantInt::get(tyI32(), ie->value);
  }
  if (auto* ve = dynamic_cast<const VarExpr*>(e)) {
    auto it = scope.find(ve->name);
    if (it == scope.end()) throw std::runtime_error("Unknown variable: " + ve->name);
    auto* ld = builder_->CreateLoad(tyI8Ptr(), it->second.allocaPtr, ve->name + ".val");
    annotate(ld, ve->loc, "load var");
    return ld;
  }
  if (auto* ne = dynamic_cast<const NewExpr*>(e)) {
    // Alloca object and set vptr
    ClassInfo& ci = requireClass(ne->className);
    auto* obj = builder_->CreateAlloca(ci.classTy, /*ArraySize=*/nullptr, ne->className + ".obj");
    annotate(obj, ne->loc, "alloca object");
    // GEP to first field (vptr)
    auto* vptrAddr = builder_->CreateStructGEP(ci.classTy, obj, 0, ne->className + ".vptr.addr");
    annotate(vptrAddr, ne->loc, "vptr addr");
    auto* st = builder_->CreateStore(ci.vtableGlobal, vptrAddr);
    annotate(st, ne->loc, "store vptr");
    return obj;
  }
  if (auto* me = dynamic_cast<const MethodCallExpr*>(e)) {
    // Only 'recv.method()' w/o args
    // Resolve receiver var type from scope
    if (auto* recvVar = dynamic_cast<const VarExpr*>(me->receiver.get())) {
      auto it = scope.find(recvVar->name);
      if (it == scope.end()) throw std::runtime_error("Unknown variable: " + recvVar->name);
      llvm::Value* thisPtr = builder_->CreateLoad(tyI8Ptr(), it->second.allocaPtr, recvVar->name + ".val");
      annotate(thisPtr, me->loc, "load this");
      return codegenVirtualCall(thisPtr, it->second.typeName, me->methodName,
                                expectedType.empty() ? std::string("String") : expectedType,
                                &me->loc);
    }
    throw std::runtime_error("Unsupported method receiver expression");
  }
  throw std::runtime_error("Unhandled expression node");
}

/// Lower a statement. Handles return, print, and variable declarations.
void CodeGen::codegenStmt(const Stmt* s,
                          std::map<std::string, ScopeVar>& scope,
                          const std::string& currentRetType,
                          llvm::StructType* currentClassTy) {
  (void)currentClassTy;
  if (auto* r = dynamic_cast<const ReturnStmt*>(s)) {
    llvm::Value* v = codegenExpr(r->value.get(), scope, currentRetType);
    auto* ret = builder_->CreateRet(v);
    annotate(ret, r->loc, "return");
    // Note: caller should ensure no further instructions are emitted after return
    return;
  }
  if (auto* p = dynamic_cast<const PrintStmt*>(s)) {
    llvm::Value* v = codegenExpr(p->value.get(), scope, "String");
    auto* call = builder_->CreateCall(getOrDeclarePuts(), {v});
    annotate(call, p->loc, "print");
    return;
  }
  if (auto* vd = dynamic_cast<const VarDeclStmt*>(s)) {
    // Variable is a pointer ('ptr') to an object (or string/int but demo uses objects)
    auto* allocaPtr = builder_->CreateAlloca(llvm::PointerType::getUnqual(tyI8Ptr()), /*ArraySize=*/nullptr, vd->name + ".addr");
    annotate(allocaPtr, vd->loc, "alloca var");
    llvm::Value* init = codegenExpr(vd->init.get(), scope, vd->type.name);
    // Store the object pointer into the variable slot (both are 'ptr' under opaque pointers)
    auto* st = builder_->CreateStore(init, allocaPtr);
    annotate(st, vd->loc, "store var");
    scope.emplace(vd->name, ScopeVar{allocaPtr, vd->type.name});
    return;
  }
  throw std::runtime_error("Unhandled statement node");
}

/// Emit a virtual call: load the vptr, read the slot for `methodName`, cast
/// the function pointer to the right type, and call it with `thisPtr`.
llvm::Value* CodeGen::codegenVirtualCall(llvm::Value* thisPtr,
                                         const std::string& staticClassName,
                                         const std::string& methodName,
                                         const std::string& retTypeName,
                                         const SourceRange* srcLoc) {
  // Load vptr: first field of class struct
  ClassInfo& ci = requireClass(staticClassName);
  auto* vptrAddr = builder_->CreateStructGEP(ci.classTy, thisPtr, 0, staticClassName + ".vptr.addr");
  if (srcLoc) annotate(vptrAddr, *srcLoc, "vptr addr");
  llvm::Value* vptr = builder_->CreateLoad(llvm::PointerType::getUnqual(ci.vtableTy), vptrAddr, staticClassName + ".vptr");
  if (srcLoc) annotate(vptr, *srcLoc, "load vptr");

  // Get function pointer from slot
  size_t slot = slotOf(staticClassName, methodName);
  auto* slotAddr = builder_->CreateStructGEP(ci.vtableTy, vptr, static_cast<unsigned>(slot), methodName + ".slot.addr");
  if (srcLoc) annotate(slotAddr, *srcLoc, "slot addr");
  llvm::Value* fnI8 = builder_->CreateLoad(tyI8Ptr(), slotAddr, methodName + ".slot");
  if (srcLoc) annotate(fnI8, *srcLoc, "load slot");

  // Cast to function pointer type and call
  auto* fnTy = methodFnTy(retTypeName, ci.classTy);
  auto* fnPtrTy = llvm::PointerType::getUnqual(fnTy);
  llvm::Value* fn = builder_->CreatePointerCast(fnI8, fnPtrTy, methodName + ".fn");
  if (srcLoc) annotate(fn, *srcLoc, "bitcast fn");
  auto* call = builder_->CreateCall(fnTy, fn, {thisPtr}, methodName + ".call");
  if (srcLoc) annotate(call, *srcLoc, "vcall");
  return call;
}

/// Lookup a ClassInfo by name or throw a diagnostic if unknown.
ClassInfo& CodeGen::requireClass(const std::string& name) {
  auto it = classes_.find(name);
  if (it == classes_.end()) throw std::runtime_error("Unknown class: " + name);
  return it->second;
}

/// Return the base class of `c` if present; otherwise nullptr.
const ClassInfo* CodeGen::maybeBaseOf(const ClassInfo& c) const {
  if (!c.base) return nullptr;
  auto it = classes_.find(*c.base);
  if (it == classes_.end()) throw std::runtime_error("Unknown base class: " + *c.base);
  return &it->second;
}

/// Return the vtable slot index for `methodName` in `className` or throw.
size_t CodeGen::slotOf(const std::string& className, const std::string& methodName) const {
  auto it = classes_.find(className);
  if (it == classes_.end()) throw std::runtime_error("Unknown class: " + className);
  auto jt = it->second.layout.slotOf.find(methodName);
  if (jt == it->second.layout.slotOf.end()) throw std::runtime_error("No virtual method '" + methodName + "' in class '" + className + "'");
  return jt->second;
}

/// Find the most-derived implementation for `methodName` starting at
/// `className`, walking up the inheritance chain as needed.
llvm::Function* CodeGen::implementationFor(const std::string& className, const std::string& methodName) {
  // Walk up to find the most-derived implementation in this class
  auto it = classes_.find(className);
  if (it == classes_.end()) throw std::runtime_error("Unknown class: " + className);
  const ClassInfo* c = &it->second;
  while (c) {
    auto mt = c->methods.find(methodName);
    if (mt != c->methods.end()) return mt->second;
    c = maybeBaseOf(*c);
  }
  throw std::runtime_error("No implementation for method '" + methodName + "' in class '" + className + "'");
}

} // namespace fakelang
