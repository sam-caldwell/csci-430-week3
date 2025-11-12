// Fakelang LLVM IR code generator (LLVM 17)
// This module lowers the AST to LLVM IR with a minimal object model:
// - Each object is a struct with a single field: a pointer to a vtable
// - Each vtable is a struct of slots (i8* function pointers)
// - Dynamic dispatch loads the slot from the vtable and calls it
// - Strings are emitted as global constants; printing uses 'puts'
#pragma once

#include "AST.h"

// Suppress deprecation warnings originating from LLVM headers under C++23
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fakelang {

/// Describes the vtable method layout for a class.
struct ClassLayout {
  /// Vtable order: slot i contains the method named methods[i].
  std::vector<std::string> methods;
  /// Map method name -> slot index.
  std::unordered_map<std::string, size_t> slotOf;
};

/// Aggregates information for codegen about a single class.
struct ClassInfo {
  /// AST node for this class declaration.
  const ClassDecl* ast{nullptr};
  /// Class name.
  std::string name;
  /// Optional base class name, if the class extends another.
  std::optional<std::string> base;
  /// Computed vtable layout, including inherited slots.
  ClassLayout layout;

  /// %class.<Name> = type { ptr }
  llvm::StructType* classTy{nullptr};
  /// %vtable.<Name> = type { i8*, ... }
  llvm::StructType* vtableTy{nullptr};
  /// @vtable.<Name>
  llvm::GlobalVariable* vtableGlobal{nullptr};

  /// Mapping from method name to the defined function in this class.
  std::unordered_map<std::string, llvm::Function*> methods;
};

/// Lowers fakelang AST to LLVM IR using LLVM 17 APIs.
class CodeGen {
public:
  /// Construct a new, empty code generator with its own context and module.
  CodeGen();

  /// Provide the original source buffer and filename for annotation purposes.
  /// This enables mapping IR back to source lines in emitted comments.
  void setSource(std::string sourceText, std::string filename);

  /// Generate an LLVM module for the given program.
  /// Ownership stays in this class; use getModule() for a non-owning pointer.
  void generate(const Program& program, const std::string& moduleName = "fakelang-module");
  llvm::Module* getModule() const { return module_.get(); }

private:
  // Passes
  /// Compute vtable layouts for all classes.
  void computeClassLayouts(const Program&);
  /// Declare opaque struct types for classes and vtables.
  void declareTypes();
  /// Declare method functions and emit their bodies.
  void declareAndDefineMethods();
  /// Emit vtable globals with initialized function pointers.
  void defineVTables();
  /// Define free functions (e.g., main).
  void defineFunctions(const Program&);

  // Helpers
  /// Cached common types in the current LLVMContext.
  llvm::Type* tyVoid();
  llvm::Type* tyI32();
  llvm::Type* tyI8();
  llvm::PointerType* tyI8Ptr();

  /// Returns the LLVM function type for a method with the given return type.
  llvm::FunctionType* methodFnTy(const std::string& retTypeName, llvm::StructType* classTy);
  /// Map fakelang type name to LLVM type.
  llvm::Type* retTypeFor(const std::string& typeName);

  /// Declare or fetch the libc `puts` function used by print().
  llvm::Function* getOrDeclarePuts();

  // Expression/statement codegen for a function/method
  struct ScopeVar {
    /// Alloca that holds a 'ptr' to the variable's value (object or string ptr).
    llvm::AllocaInst* allocaPtr{nullptr};
    /// Static type of the variable (e.g., class name or "String").
    std::string typeName;
  };

  /// Lower an expression and return the resulting LLVM value.
  llvm::Value* codegenExpr(const Expr*, std::map<std::string, ScopeVar>& scope,
                           const std::string& expectedType = "");
  /// Lower a statement inside a function or method body.
  void codegenStmt(const Stmt*, std::map<std::string, ScopeVar>& scope,
                   const std::string& currentRetType,
                   llvm::StructType* currentClassTy);

  // Dynamic dispatch helper
  /// Perform a virtual call through the vtable of `staticClassName` using
  /// the given method name, returning the call result value.
  llvm::Value* codegenVirtualCall(llvm::Value* thisPtr,
                                  const std::string& staticClassName,
                                  const std::string& methodName,
                                  const std::string& retTypeName,
                                  const SourceRange* srcLoc = nullptr);

  // Utilities
  /// Lookup a class by name or throw if unknown.
  ClassInfo& requireClass(const std::string& name);
  /// If `c` has a base class, return its ClassInfo; otherwise nullptr.
  const ClassInfo* maybeBaseOf(const ClassInfo& c) const;
  /// Return the vtable slot index for a class/method pair.
  size_t slotOf(const std::string& className, const std::string& methodName) const;
  /// Resolve the most-derived implementation function for a class/method pair.
  llvm::Function* implementationFor(const std::string& className, const std::string& methodName);

  // State
  llvm::LLVMContext ctx_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;

  // name -> ClassInfo
  std::map<std::string, ClassInfo> classes_;

  // Source (for annotation)
  std::string sourceFilename_{};
  std::string sourceText_{};
  std::vector<std::string> sourceLines_{}; // 1-based lines stored 0-based here

  // Annotation helpers
  void annotate(llvm::Value* v, const SourceRange& rng, std::string_view kind);
  std::string srcSnippet(const SourceRange& rng) const;
};

} // namespace fakelang
