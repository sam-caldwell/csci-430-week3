#include "Lexer.h"
#include "Parser.h"
#include <gtest/gtest.h>

using namespace fakelang;

TEST(Parser, ClassAndFunction) {
  const char* src = R"(
    class Animal { virtual speak(): String { return "Animal"; } }
    class Dog extends Animal { override speak(): String { return "Woof"; } }
    function main(): Int { var a: Animal = new Animal(); var d: Animal = new Dog(); return 0; }
  )";
  Lexer lex(src);
  auto toks = lex.lexAll();
  Parser p(std::move(toks));
  Program prog = p.parseProgram();
  ASSERT_EQ(prog.classes.size(), 2u);
  EXPECT_EQ(prog.classes[0].name, "Animal");
  EXPECT_EQ(prog.classes[1].name, "Dog");
  ASSERT_TRUE(prog.classes[1].baseName.has_value());
  EXPECT_EQ(*prog.classes[1].baseName, "Animal");
  ASSERT_EQ(prog.functions.size(), 1u);
  EXPECT_EQ(prog.functions[0].name, "main");
}

