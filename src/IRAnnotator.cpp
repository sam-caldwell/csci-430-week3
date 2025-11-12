#include "IRAnnotator.h"
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
#include <llvm/IR/Function.h>
#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

namespace fakelang {

void FakelangAnnotationWriter::emitFunctionAnnot(const llvm::Function* F,
                                                 llvm::formatted_raw_ostream& OS) {
  OS << ";\n; === Function: " << F->getName() << " ===\n";
}

void FakelangAnnotationWriter::emitInstructionAnnot(const llvm::Instruction* I,
                                                    llvm::formatted_raw_ostream& OS) {
  if (auto const* md = I->getMetadata("fakelang.src")) {
    if (md->getNumOperands() > 0) {
      if (auto const* s = llvm::dyn_cast<llvm::MDString>(md->getOperand(0))) {
        OS << " ; src: " << s->getString();
      }
    }
  }
}

} // namespace fakelang
