// Fakelang Lexer: converts source text into a stream of tokens.
// Focuses on clarity over performance; suitable for teaching.
#pragma once

#include "Token.h"
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fakelang {

/// Lexical analyzer for fakelang.
///
/// The lexer scans a UTF-8 string and emits a flat sequence of tokens
/// including a final Eof token. It recognizes line comments starting with
/// "//" and a handful of keywords and punctuation. Whitespace is skipped.
class Lexer {
public:
  /// Construct a lexer for a given input buffer.
  ///
  /// - input: full source buffer to lex
  /// - filename: used for diagnostics only
  explicit Lexer(std::string_view input, std::string filename = "<input>")
      : input_(input), filename_(std::move(filename)) {}

  /// Lex the full input into a vector of tokens (includes a final Eof token).
  /// Throws std::runtime_error on malformed lexemes (e.g., unterminated string).
  std::vector<Token> lexAll();

private:
  /// Peek at the current character without consuming it; returns '\0' at end.
  char peek() const { return (pos_ < input_.size()) ? input_[pos_] : '\0'; }
  /// Get and consume the current character; advances the SourcePos.
  char get();
  /// Advance by one character (convenience wrapper around get()).
  void advance() { (void)get(); }

  /// Returns true if `c` can start an identifier.
  static bool isIdentStart(char c);
  /// Returns true if `c` can continue an identifier.
  static bool isIdentChar(char c);

  /// Skip whitespace and line comments.
  void skipWhitespaceAndComments();
  /// Construct a token with range [start, cur_).
  Token makeToken(TokenKind kind, std::string text, SourcePos start);
  /// Lex an identifier or a keyword starting at `start`.
  Token lexIdentifierOrKeyword(SourcePos start);
  /// Lex a decimal integer starting at `start`.
  Token lexNumber(SourcePos start);
  /// Lex a double-quoted string literal starting at `start`.
  Token lexString(SourcePos start);

  /// Backing source buffer (not owned).
  std::string_view input_{};
  /// Filename used in diagnostics.
  std::string filename_{};
  /// Current index into `input_`.
  size_t pos_{0};
  /// Current source position (1-based line/column).
  SourcePos cur_{1, 1};
};

} // namespace fakelang
