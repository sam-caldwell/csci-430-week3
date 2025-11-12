// Fakelang IR annotator: prints labeled sections and source mapping comments
#pragma once

// Suppress deprecation warnings originating from LLVM headers under C++23
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
#include <llvm/IR/AssemblyAnnotationWriter.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Metadata.h>
#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

namespace fakelang {

class FakelangAnnotationWriter final : public llvm::AssemblyAnnotationWriter {
public:
  void emitFunctionAnnot(const llvm::Function* F,
                         llvm::formatted_raw_ostream& OS) override;

  void emitInstructionAnnot(const llvm::Instruction* I,
                            llvm::formatted_raw_ostream& OS) override;
};

} // namespace fakelang

