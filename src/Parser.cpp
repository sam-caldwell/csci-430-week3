#include "Parser.h"

#include <cstdlib>
#include <stdexcept>

namespace fakelang {

/// Return a reference to the token at relative offset i from the current
/// position (0 = current). If out of range, returns the final Eof token.
const Token& Parser::peek(size_t i) const {
  if (pos_ + i >= tokens_.size()) return tokens_.back();
  return tokens_[pos_ + i];
}

/// Consume and return a token of the given kind. Throws a descriptive
/// std::runtime_error if the next token does not match.
const Token& Parser::expect(TokenKind k, const char* what) {
  const Token& t = peek();
  if (t.kind != k) {
    throw std::runtime_error(std::string("Expected ") + what + ", found " + to_string(t.kind));
  }
  pos_++;
  return t;
}

/// If the next token matches k, consume and return true; else return false.
bool Parser::consumeIf(TokenKind k) {
  if (peek().kind == k) { pos_++; return true; }
  return false;
}

/// Expect an identifier and return its text; throws otherwise.
std::string Parser::expectIdent(const char* what) {
  const Token& t = expect(TokenKind::Identifier, what);
  return t.text;
}

/// Parse a type reference (an identifier). For this demo, any identifier is
/// accepted and interpreted by codegen.
TypeRef Parser::parseType() {
  // Only 'Int', 'String', or class names for this demo; we accept any identifier
  std::string name = expectIdent("type name");
  return TypeRef{name};
}

/// Parse a sequence of class/function declarations until Eof.
Program Parser::parseProgram() {
  Program p;
  while (!is(TokenKind::Eof)) {
    if (is(TokenKind::KwClass)) {
      p.classes.push_back(parseClassDecl());
    } else if (is(TokenKind::KwFunction)) {
      p.functions.push_back(parseFunctionDecl());
    } else {
      throw std::runtime_error("Expected 'class' or 'function'");
    }
  }
  return p;
}

/// Parse a class declaration with optional 'extends Base'.
ClassDecl Parser::parseClassDecl() {
  const Token& tClass = expect(TokenKind::KwClass, "'class'");
  ClassDecl c;
  c.name = expectIdent("class name");
  if (consumeIf(TokenKind::KwExtends)) {
    c.baseName = expectIdent("base class name");
  }
  expect(TokenKind::LBrace, "'{'");
  while (!is(TokenKind::RBrace)) {
    c.methods.push_back(parseMethod());
  }
  const Token& tR = expect(TokenKind::RBrace, "'}'");
  c.loc = SourceRange{tClass.range.start, tR.range.end};
  return c;
}

/// Parse a method declaration with optional 'virtual' or 'override' modifier.
MethodDecl Parser::parseMethod() {
  MethodDecl m;
  const Token& tStart = peek();
  if (consumeIf(TokenKind::KwVirtual)) m.attr = MethodAttr::Virtual;
  else if (consumeIf(TokenKind::KwOverride)) m.attr = MethodAttr::Override;
  m.name = expectIdent("method name");
  expect(TokenKind::LParen, "'('");
  expect(TokenKind::RParen, "')'");
  expect(TokenKind::Colon, "':'");
  m.returnType = parseType();
  expect(TokenKind::LBrace, "'{'");
  while (!is(TokenKind::RBrace)) m.body.push_back(parseStmt());
  const Token& tEnd = expect(TokenKind::RBrace, "'}'");
  m.loc = SourceRange{tStart.range.start, tEnd.range.end};
  return m;
}

/// Parse a free function declaration. The demo expects 'main' only.
FunctionDecl Parser::parseFunctionDecl() {
  const Token& tFun = expect(TokenKind::KwFunction, "'function'");
  FunctionDecl f;
  f.name = expectIdent("function name");
  expect(TokenKind::LParen, "'('");
  expect(TokenKind::RParen, "')'");
  expect(TokenKind::Colon, "':'");
  f.returnType = parseType();
  expect(TokenKind::LBrace, "'{'");
  while (!is(TokenKind::RBrace)) f.body.push_back(parseStmt());
  const Token& tEnd = expect(TokenKind::RBrace, "'}'");
  f.loc = SourceRange{tFun.range.start, tEnd.range.end};
  return f;
}

/// Parse a single statement. Only 'var', 'print', and 'return' are supported.
std::unique_ptr<Stmt> Parser::parseStmt() {
  if (is(TokenKind::KwVar)) return parseVarDecl();
  if (is(TokenKind::KwPrint)) return parsePrint();
  if (is(TokenKind::KwReturn)) return parseReturn();
  throw std::runtime_error("Unexpected token in statement");
}

/// Parse 'var name: Type = new Class();' followed by a semicolon.
std::unique_ptr<Stmt> Parser::parseVarDecl() {
  const Token& tVar = expect(TokenKind::KwVar, "'var'");
  auto s = std::make_unique<VarDeclStmt>();
  s->name = expectIdent("variable name");
  expect(TokenKind::Colon, "':'");
  s->type = parseType();
  expect(TokenKind::Assign, "'='");
  s->init = parseNewExpr();
  const Token& tSemi = expect(TokenKind::Semicolon, "';'");
  s->loc = SourceRange{tVar.range.start, tSemi.range.end};
  return s;
}

/// Parse 'print(expr);'.
std::unique_ptr<Stmt> Parser::parsePrint() {
  const Token& tPrint = expect(TokenKind::KwPrint, "'print'");
  expect(TokenKind::LParen, "'('");
  auto s = std::make_unique<PrintStmt>();
  s->value = parseExpr();
  expect(TokenKind::RParen, "')'");
  const Token& tSemi = expect(TokenKind::Semicolon, "';'");
  s->loc = SourceRange{tPrint.range.start, tSemi.range.end};
  return s;
}

/// Parse 'return expr;'.
std::unique_ptr<Stmt> Parser::parseReturn() {
  const Token& tRet = expect(TokenKind::KwReturn, "'return'");
  auto s = std::make_unique<ReturnStmt>();
  s->value = parseExpr();
  const Token& tSemi = expect(TokenKind::Semicolon, "';'");
  s->loc = SourceRange{tRet.range.start, tSemi.range.end};
  return s;
}

/// Parse an expression. The grammar has only primaries in this demo.
std::unique_ptr<Expr> Parser::parseExpr() {
  // For this demo, expressions are just primaries (no binary ops needed)
  return parsePrimary();
}

/// Parse a primary: string, number, 'new' or identifier.
std::unique_ptr<Expr> Parser::parsePrimary() {
  if (is(TokenKind::String)) {
    const Token& t = peek();
    std::string v = t.text; pos_++;
    auto e = std::make_unique<StringExpr>(); e->value = std::move(v); e->loc = t.range; return e;
  }
  if (is(TokenKind::Number)) {
    const Token& t = peek();
    int v = std::stoi(t.text); pos_++;
    auto e = std::make_unique<IntExpr>(); e->value = v; e->loc = t.range; return e;
  }
  if (is(TokenKind::KwNew)) return parseNewExpr();
  if (is(TokenKind::Identifier)) return parseMethodCallOrVar();
  throw std::runtime_error("Unexpected token in expression");
}

/// Parse 'new Class()'.
std::unique_ptr<Expr> Parser::parseNewExpr() {
  const Token& tNew = expect(TokenKind::KwNew, "'new'");
  std::string cls = expectIdent("class name");
  expect(TokenKind::LParen, "'('");
  const Token& tRP = expect(TokenKind::RParen, "')'");
  auto e = std::make_unique<NewExpr>();
  e->className = std::move(cls);
  e->loc = SourceRange{tNew.range.start, tRP.range.end};
  return e;
}

/// Parse either a variable reference or a zero-arg method call on a variable.
std::unique_ptr<Expr> Parser::parseMethodCallOrVar() {
  // Start with identifier
  const Token& tIdent = expect(TokenKind::Identifier, "identifier");
  std::string ident = tIdent.text;
  if (consumeIf(TokenKind::Dot)) {
    std::string method = expectIdent("method name");
    expect(TokenKind::LParen, "'('");
    const Token& tRP = expect(TokenKind::RParen, "')'");
    auto recv = std::make_unique<VarExpr>();
    recv->name = std::move(ident);
    recv->loc = tIdent.range;
    auto call = std::make_unique<MethodCallExpr>();
    call->receiver = std::move(recv);
    call->methodName = std::move(method);
    call->loc = SourceRange{tIdent.range.start, tRP.range.end};
    return call;
  }
  auto v = std::make_unique<VarExpr>(); v->name = std::move(ident); v->loc = tIdent.range; return v;
}

} // namespace fakelang
