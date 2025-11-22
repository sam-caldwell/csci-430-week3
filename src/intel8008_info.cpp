#include "Intel8008MCTargetDesc.h"

#include <algorithm>
#include <iostream>

int main() {
  using namespace intel8008;
  const auto regs = getAllRegisterNames();
  const auto insn = getAllInstructionNames();

  std::cout << "Intel8008 registers (" << regs.size() << ")\n";
  for (const auto &r : regs) std::cout << "  " << r << "\n";

  std::cout << "\nIntel8008 instructions (" << insn.size() << ")\n";
  for (const auto &i : insn) std::cout << "  " << i << "\n";

  return 0;
}
