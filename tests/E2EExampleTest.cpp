#include "Lexer.h"
#include "Parser.h"
#include "CodeGen.h"

#include <gtest/gtest.h>
#include <fstream>
#include <sstream>

using namespace fakelang;

static std::string readAll(const std::string& path) {
  std::ifstream in(path);
  std::ostringstream ss; ss << in.rdbuf();
  return ss.str();
}

TEST(E2E, DemoExampleCompilesToIR) {
  std::string src = readAll(std::string(FAKELANG_SOURCE_DIR) + "/demo/example.fakelang");
  Lexer lex(src, "demo/example.fakelang");
  auto toks = lex.lexAll();
  Parser p(std::move(toks));
  Program prog = p.parseProgram();
  CodeGen cg; cg.generate(prog, "demo");
  std::string s; llvm::raw_string_ostream os(s); os << *cg.getModule();
  auto ir = os.str();
  EXPECT_NE(ir.find("vtable.Dog"), std::string::npos);
  EXPECT_NE(ir.find("puts"), std::string::npos);
}
