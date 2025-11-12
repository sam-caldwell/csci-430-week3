#include "Lexer.h"
#include "Parser.h"
#include "CodeGen.h"

#include <gtest/gtest.h>
#include <string>

using namespace fakelang;

static std::string toString(llvm::Module* m) {
  std::string s; llvm::raw_string_ostream os(s); os << *m; return os.str();
}

TEST(CodeGen, EmitsVTablesAndMethods) {
  const char* src = R"(
    class Animal { virtual speak(): String { return "Animal"; } }
    class Dog extends Animal { override speak(): String { return "Woof"; } }
    function main(): Int { var a: Animal = new Animal(); var d: Animal = new Dog(); print(a.speak()); print(d.speak()); return 0; }
  )";
  Lexer lex(src);
  auto toks = lex.lexAll();
  Parser p(std::move(toks));
  Program prog = p.parseProgram();

  CodeGen cg; cg.generate(prog, "test");
  std::string ir = toString(cg.getModule());
  // Look for key artifacts
  EXPECT_NE(ir.find("vtable.Animal"), std::string::npos);
  EXPECT_NE(ir.find("vtable.Dog"), std::string::npos);
  EXPECT_NE(ir.find("Animal.speak"), std::string::npos);
  EXPECT_NE(ir.find("Dog.speak"), std::string::npos);
  EXPECT_NE(ir.find("declare i32 @puts"), std::string::npos);
}

