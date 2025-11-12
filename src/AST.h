// Fakelang AST definitions
// A minimal, readable AST to support classes, methods, and a small main.
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Token.h" // for SourceRange

namespace fakelang {

/// Type reference used by AST nodes.
/// Types are referenced by name (e.g., "Int", "String", or a class name).
struct TypeRef {
  std::string name;
};

// Forward declarations
struct Expr;
struct Stmt;

/// Base class for all expression nodes.
struct Expr {
  virtual ~Expr() = default;
  // Source range covering this expression in the original file
  SourceRange loc{};
};

/// A string literal expression "..."
struct StringExpr : Expr {
  std::string value;
};

/// An integer literal expression (only used for return 0 in example).
struct IntExpr : Expr {
  int value{};
};

/// Variable reference expression: `a`.
struct VarExpr : Expr {
  std::string name;
};

/// Object creation expression: `new ClassName()`.
struct NewExpr : Expr {
  std::string className;
};

/// Virtual method call with no arguments: `<recv>.<method>()`.
struct MethodCallExpr : Expr {
  std::unique_ptr<Expr> receiver;
  std::string methodName;
};

/// Base class for all statement nodes.
struct Stmt {
  virtual ~Stmt() = default;
  // Source range covering this statement (including trailing semicolon)
  SourceRange loc{};
};

/// Return statement: `return expr;`.
struct ReturnStmt : Stmt {
  std::unique_ptr<Expr> value; // may be nullptr for 'return;'
};

/// Print statement: `print(expr);` emits a call to `puts` at codegen.
struct PrintStmt : Stmt {
  std::unique_ptr<Expr> value; // expects string at runtime
};

/// Variable declaration: `var name: Type = init;`.
struct VarDeclStmt : Stmt {
  std::string name;
  TypeRef type;
  std::unique_ptr<Expr> init; // e.g., 'new Class()'
};

/// Method attribute: either none, virtual, or override.
enum class MethodAttr { None, Virtual, Override };

/// Method declaration inside a class. For the demo, methods have no parameters
/// and a single return type.
struct MethodDecl {
  MethodAttr attr{MethodAttr::None};
  std::string name;
  TypeRef returnType;
  std::vector<std::unique_ptr<Stmt>> body;
  // Source range from the first token of the method header to the closing brace
  SourceRange loc{};
};

/// Class declaration with an optional base class and zero or more methods.
struct ClassDecl {
  std::string name;
  std::optional<std::string> baseName; // 'extends X'
  std::vector<MethodDecl> methods;
  // Source range from 'class' to the closing brace
  SourceRange loc{};
};

/// Free function (only 'main' is expected for the demo).
struct FunctionDecl {
  std::string name;
  TypeRef returnType;
  std::vector<std::unique_ptr<Stmt>> body;
  // Source range from 'function' to the closing brace
  SourceRange loc{};
};

/// Root of the AST: a sequence of classes and free functions.
struct Program {
  std::vector<ClassDecl> classes;
  std::vector<FunctionDecl> functions;
};

} // namespace fakelang
