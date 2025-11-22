//===- Intel8008MCTargetDesc.h -------------------------------*- C++ -*-===//
// Minimal MC-descriptor helpers for the demo Intel8008 target.
// Consumes TableGen-generated .inc files for enums and MC descriptors.
//===----------------------------------------------------------------------===//

#pragma once

#include <string>
#include <vector>

namespace llvm { class MCInstrInfo; }

namespace intel8008 {

// Convenience helpers for demos/tests.
std::vector<std::string> getAllRegisterNames();
void initMCInstrInfo(llvm::MCInstrInfo &II);
std::vector<std::string> getAllInstructionNames();

} // namespace intel8008
