// Fakelang Parser: builds an AST from tokens.
// Grammar (informal):
//   program      := (classDecl | functionDecl)* EOF
//   classDecl    := 'class' Ident ('extends' Ident)? '{' method* '}'
//   method       := ('virtual'|'override')? Ident '(' ')' ':' Type '{' stmt* '}'
//   functionDecl := 'function' Ident '(' ')' ':' Type '{' stmt* '}'
//   stmt         := varDecl ';' | print ';' | return ';'
//   varDecl      := 'var' Ident ':' Type '=' 'new' Ident '(' ')'
//   print        := 'print' '(' expr ')'
//   return       := 'return' expr
//   expr         := String | Number | Ident | newExpr | methodCall
//   newExpr      := 'new' Ident '(' ')'
//   methodCall   := Ident '.' Ident '(' ')'
#pragma once

#include "AST.h"
#include "Token.h"
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fakelang {

/// Handwritten recursive-descent parser for fakelang.
///
/// The parser aims to be straightforward and explicit for instructional
/// value. It performs minimal semantic checks; most errors are caught
/// during code generation.
class Parser {
public:
  /// Create a parser over a full token stream (including Eof).
  explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

  /// Parse an entire program consisting of class and function declarations.
  Program parseProgram();

private:
  /// Lookahead accessor; returns the i-th token from current position.
  const Token& peek(size_t i = 0) const;
  /// True if token i matches kind k.
  bool is(TokenKind k, size_t i = 0) const { return peek(i).kind == k; }
  /// Consume and return a token of kind k, or throw with a helpful message.
  const Token& expect(TokenKind k, const char* what);
  /// If next token matches k, consume it and return true; otherwise false.
  bool consumeIf(TokenKind k);

  /// Expect and return an identifier token's text.
  std::string expectIdent(const char* what);
  /// Parse a type reference (identifier name).
  TypeRef parseType();

  /// Parse a class declaration.
  ClassDecl parseClassDecl();
  /// Parse a method declaration within a class.
  MethodDecl parseMethod();
  /// Parse a free function declaration.
  FunctionDecl parseFunctionDecl();

  /// Parse a single statement.
  std::unique_ptr<Stmt> parseStmt();
  /// Parse a variable declaration statement.
  std::unique_ptr<Stmt> parseVarDecl();
  /// Parse a print statement.
  std::unique_ptr<Stmt> parsePrint();
  /// Parse a return statement.
  std::unique_ptr<Stmt> parseReturn();

  /// Parse an expression (no precedence; only primaries in this demo).
  std::unique_ptr<Expr> parseExpr();
  /// Parse literals/new/identifier primaries.
  std::unique_ptr<Expr> parsePrimary();
  /// Parse `new Class()` expression.
  std::unique_ptr<Expr> parseNewExpr();
  /// Parse a variable or a zero-argument method call on a variable.
  std::unique_ptr<Expr> parseMethodCallOrVar();

  std::vector<Token> tokens_;
  size_t pos_{0};
};

} // namespace fakelang
