#include "Lexer.h"
#include "Parser.h"
#include "CodeGen.h"
#include "IRAnnotator.h"

#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace fakelang;

/// Read the entire contents of `path` into a string or throw on failure.
static std::string readFile(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("Failed to open input file: " + path);
  std::ostringstream ss; ss << in.rdbuf();
  return ss.str();
}

/// Print a short usage message to stderr.
static void usage(const char* argv0) {
  std::cerr << "Usage: " << argv0 << " <input.fakelang> [-o <output.ll|->]\n";
}

/// CLI entrypoint: lex, parse, and lower the input program to LLVM IR.
int main(int argc, char** argv) {
  if (argc < 2) { usage(argv[0]); return 1; }
  std::string input = argv[1];
  std::string output = "-"; // default to stdout
  for (int i = 2; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-o" && i + 1 < argc) { output = argv[++i]; }
    else if (arg == "-h" || arg == "--help") { usage(argv[0]); return 0; }
    else { std::cerr << "Unknown argument: " << arg << "\n"; usage(argv[0]); return 1; }
  }

  try {
    std::string src = readFile(input);

    Lexer lex(src, input);
    auto tokens = lex.lexAll();

    Parser parser(std::move(tokens));
    Program prog = parser.parseProgram();

    CodeGen cg;
    cg.setSource(src, input);
    cg.generate(prog, input);

    auto printWithAnnotations = [&](llvm::raw_ostream& os){
      // Section: Source (as comments)
      os << "; === Source: " << input << " ===\n";
      std::istringstream iss(src);
      std::string line; size_t ln = 1;
      while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        os << "; " << ln++ << " | " << line << "\n";
      }
      os << "; === LLVM Module IR ===\n";
      fakelang::FakelangAnnotationWriter annot;
      cg.getModule()->print(os, &annot);
    };

    if (output == "-" || output.empty()) {
      printWithAnnotations(llvm::outs());
    } else {
      std::error_code ec;
      llvm::raw_fd_ostream os(output, ec, llvm::sys::fs::OF_None);
      if (ec) throw std::runtime_error("Failed to open output: " + ec.message());
      printWithAnnotations(os);
    }
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "error: " << ex.what() << "\n";
    return 2;
  }
}
