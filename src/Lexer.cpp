#include "Lexer.h"

#include <cctype>
#include <stdexcept>

namespace fakelang {

/// Consume and return the current character, updating the 1-based line/column
/// counters. Returns '\0' when positioned at end-of-input.
char Lexer::get() {
  if (pos_ >= input_.size()) return '\0';
  char c = input_[pos_++];
  if (c == '\n') {
    cur_.line++;
    cur_.column = 1;
  } else {
    cur_.column++;
  }
  return c;
}

/// Returns true if `c` can start an identifier (alpha or underscore).
bool Lexer::isIdentStart(char c) {
  return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

/// Returns true if `c` can continue an identifier (alnum or underscore).
bool Lexer::isIdentChar(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

/// Skips spaces, tabs, newlines, and line comments beginning with "//".
void Lexer::skipWhitespaceAndComments() {
  while (true) {
    char c = peek();
    // Whitespace
    while (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      advance();
      c = peek();
    }
    // Line comments: // ... end of line
    if (c == '/' && pos_ + 1 < input_.size() && input_[pos_ + 1] == '/') {
      while (peek() != '\n' && peek() != '\0') advance();
      continue; // loop to consume trailing whitespace
    }
    break;
  }
}

/// Constructs a token with the given kind/text and the half-open range
/// starting at `start` and ending at the current `cur_` position.
Token Lexer::makeToken(TokenKind kind, std::string text, SourcePos start) {
  return Token{kind, std::move(text), SourceRange{start, cur_}};
}

/// Lex an identifier or a reserved keyword. The first character was already
/// consumed by the caller.
Token Lexer::lexIdentifierOrKeyword(SourcePos start) {
  std::string s;
  s.push_back(input_[pos_ - 1]); // first char already consumed
  while (isIdentChar(peek())) s.push_back(get());

  std::string_view sv{s};
  if (sv == "class") return makeToken(TokenKind::KwClass, s, start);
  if (sv == "extends") return makeToken(TokenKind::KwExtends, s, start);
  if (sv == "function") return makeToken(TokenKind::KwFunction, s, start);
  if (sv == "virtual") return makeToken(TokenKind::KwVirtual, s, start);
  if (sv == "override") return makeToken(TokenKind::KwOverride, s, start);
  if (sv == "var") return makeToken(TokenKind::KwVar, s, start);
  if (sv == "return") return makeToken(TokenKind::KwReturn, s, start);
  if (sv == "new") return makeToken(TokenKind::KwNew, s, start);
  if (sv == "print") return makeToken(TokenKind::KwPrint, s, start);
  return makeToken(TokenKind::Identifier, s, start);
}

/// Lex a decimal integer literal. The first digit was already consumed.
Token Lexer::lexNumber(SourcePos start) {
  std::string s;
  s.push_back(input_[pos_ - 1]);
  while (std::isdigit(static_cast<unsigned char>(peek()))) s.push_back(get());
  return makeToken(TokenKind::Number, s, start);
}

/// Lex a double-quoted string literal with minimal escape support.
/// Supported escapes: \n, \t, \r, ", \\.
Token Lexer::lexString(SourcePos start) {
  std::string s;
  // Previous char was opening quote already consumed
  while (true) {
    const char c = get();
    if (c == '\0') {
      throw std::runtime_error("Unterminated string literal");
    }
    if (c == '"') break;
    if (c == '\\') {
      switch (const char n = get()) {
        case 'n': s.push_back('\n'); break;
        case 't': s.push_back('\t'); break;
        case 'r': s.push_back('\r'); break;
        case '"': s.push_back('"'); break;
        case '\\': s.push_back('\\'); break;
        default: s.push_back(n); break; // minimal escapes
      }
    } else {
      s.push_back(c);
    }
  }
  return makeToken(TokenKind::String, s, start);
}

/// Lex the entire input buffer into a flat vector of tokens. The returned
/// vector always ends with an explicit Eof token.
std::vector<Token> Lexer::lexAll() {
  std::vector<Token> out;
  skipWhitespaceAndComments();
  while (true) {
    const SourcePos start = cur_;
    switch (const char c = get()) {
      case '\0':
        out.push_back(makeToken(TokenKind::Eof, "", start));
        return out;
      case '{': out.push_back(makeToken(TokenKind::LBrace, "{", start)); break;
      case '}': out.push_back(makeToken(TokenKind::RBrace, "}", start)); break;
      case '(': out.push_back(makeToken(TokenKind::LParen, "(", start)); break;
      case ')': out.push_back(makeToken(TokenKind::RParen, ")", start)); break;
      case ':': out.push_back(makeToken(TokenKind::Colon, ":", start)); break;
      case ';': out.push_back(makeToken(TokenKind::Semicolon, ";", start)); break;
      case '.': out.push_back(makeToken(TokenKind::Dot, ".", start)); break;
      case ',': out.push_back(makeToken(TokenKind::Comma, ",", start)); break;
      case '=': out.push_back(makeToken(TokenKind::Assign, "=", start)); break;
      case '"': out.push_back(lexString(start)); break;
      default:
        if (isIdentStart(c)) {
          out.push_back(lexIdentifierOrKeyword(start));
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
          out.push_back(lexNumber(start));
        } else if (std::isspace(static_cast<unsigned char>(c))) {
          // Handled by skipWhitespaceAndComments, but keep safe
        } else {
          throw std::runtime_error("Unexpected character in input");
        }
    }
    skipWhitespaceAndComments();
  }
}

} // namespace fakelang
