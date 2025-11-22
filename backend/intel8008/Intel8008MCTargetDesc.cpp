//===- Intel8008MCTargetDesc.cpp ----------------------------*- C++ -*-===//
// Minimal MC-descriptor helpers for the demo Intel8008 target.
// Uses TableGen-generated data to expose names via MC*Info.
//===----------------------------------------------------------------------===//

#include "Intel8008MCTargetDesc.h"

#include <algorithm>
#include <set>

#include <string>
#include "llvm/MC/MCInstrInfo.h"

using namespace std;

// Pull in only the register enum values and the instruction info.
#define GET_REGINFO_ENUM
#include "Intel8008GenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#include "Intel8008GenInstrInfo.inc"
// Bridge RegClassID enum names into llvm::Intel8008 to satisfy generated code.
namespace llvm { namespace Intel8008 {
enum { R8RegClassID = ::llvm::R8RegClassID, R16RegClassID = ::llvm::R16RegClassID };
} }
#define GET_INSTRINFO_MC_DESC
#include "Intel8008GenInstrInfo.inc"

namespace intel8008 {

void initMCInstrInfo(llvm::MCInstrInfo &II) {
  InitIntel8008MCInstrInfo(&II);
}

std::vector<std::string> getAllInstructionNames() {
  llvm::MCInstrInfo II;
  initMCInstrInfo(II);
  std::vector<std::string> out;
  const unsigned NOpc = II.getNumOpcodes();
  out.reserve(NOpc);
  for (unsigned opc = 0; opc < NOpc; ++opc) {
    auto N = II.getName(opc);
    if (!N.empty()) out.emplace_back(N.str());
  }
  // Leave names unsorted to match TableGen order (opcodes first, then our insts).
  return out;
}

std::vector<std::string> getAllRegisterNames() {
  // Manually map enum order -> human-readable names.
  // Order must match the enum emitted by GET_REGINFO_ENUM.
  static const char *kNames[] = {
      /*0*/ "",
      /*1*/ "A",
      /*2*/ "B",
      /*3*/ "BC",
      /*4*/ "C",
      /*5*/ "D",
      /*6*/ "DE",
      /*7*/ "E",
      /*8*/ "H",
      /*9*/ "HL",
      /*10*/ "L",
      /*11*/ "PC",
      /*12*/ "FLAGS",
      /*13*/ "RSP",
      /*14*/ "RSTK0",
      /*15*/ "RSTK1",
      /*16*/ "RSTK2",
      /*17*/ "RSTK3",
      /*18*/ "RSTK4",
      /*19*/ "RSTK5",
      /*20*/ "RSTK6",
  };
  std::vector<std::string> out;
  for (unsigned i = 1; i < sizeof(kNames) / sizeof(kNames[0]); ++i)
    out.emplace_back(kNames[i]);
  return out;
}

} // namespace intel8008
