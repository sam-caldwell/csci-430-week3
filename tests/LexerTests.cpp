#include "Lexer.h"
#include "Token.h"
#include <gtest/gtest.h>

using namespace fakelang;

TEST(Lexer, BasicTokens) {
  const char* src = R"(
    class Animal {
      virtual speak(): String { return "Animal"; }
    }
    function main(): Int { return 0; }
  )";
  Lexer lex(src);
  auto toks = lex.lexAll();
  ASSERT_GE(toks.size(), 10u);
  EXPECT_EQ(toks[0].kind, TokenKind::KwClass);
  EXPECT_EQ(toks[1].kind, TokenKind::Identifier);
  EXPECT_EQ(toks[2].kind, TokenKind::LBrace);
  // Find the string literal token
  bool foundString = false;
  for (const auto& t : toks) if (t.kind == TokenKind::String && t.text == "Animal") foundString = true;
  EXPECT_TRUE(foundString);
  EXPECT_EQ(toks.back().kind, TokenKind::Eof);
}

