// Fakelang Token definitions
// Thoroughly documented for instructional purposes.
#pragma once

#include <string>
#include <string_view>

namespace fakelang {

/// Represents a point location within an input source file.
/// Line and column are 1-based to match typical editor/diagnostic conventions.
struct SourcePos {
  int line{1};
  int column{1};
};

/// Represents a half-open source range [start, end).
/// The start/end positions are used for diagnostics and tooling.
struct SourceRange {
  SourcePos start{};
  SourcePos end{};
};

/// TokenKind enumerates the lexical atoms of fakelang.
/// These are produced by the lexer and consumed by the parser.
enum class TokenKind {
  Eof = 0,
  Identifier,
  Number,
  String,

  // Keywords
  KwClass,
  KwExtends,
  KwFunction,
  KwVirtual,
  KwOverride,
  KwVar,
  KwReturn,
  KwNew,
  KwPrint,

  // Punctuation
  LBrace,   // {
  RBrace,   // }
  LParen,   // (
  RParen,   // )
  Colon,    // :
  Semicolon,// ;
  Dot,      // .
  Comma,    // ,
  Assign    // =
};

/// Converts a TokenKind to a human-friendly string for diagnostics and errors.
inline std::string to_string(TokenKind k) {
  switch (k) {
    case TokenKind::Eof: return "<eof>";
    case TokenKind::Identifier: return "identifier";
    case TokenKind::Number: return "number";
    case TokenKind::String: return "string";
    case TokenKind::KwClass: return "class";
    case TokenKind::KwExtends: return "extends";
    case TokenKind::KwFunction: return "function";
    case TokenKind::KwVirtual: return "virtual";
    case TokenKind::KwOverride: return "override";
    case TokenKind::KwVar: return "var";
    case TokenKind::KwReturn: return "return";
    case TokenKind::KwNew: return "new";
    case TokenKind::KwPrint: return "print";
    case TokenKind::LBrace: return "{";
    case TokenKind::RBrace: return "}";
    case TokenKind::LParen: return "(";
    case TokenKind::RParen: return ")";
    case TokenKind::Colon: return ":";
    case TokenKind::Semicolon: return ";";
    case TokenKind::Dot: return ".";
    case TokenKind::Comma: return ",";
    case TokenKind::Assign: return "=";
  }
  return "<unknown>";
}

/// A Token is a single lexeme with its kind, spelling, and source location.
/// - kind: the token category
/// - text: exact source spelling (useful for identifiers/literals)
/// - range: where in the file this token came from
struct Token {
  TokenKind kind{TokenKind::Eof};
  std::string text{}; // exact spelling as it appeared in source
  SourceRange range{};
};

} // namespace fakelang
