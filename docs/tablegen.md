**Summary**
- Goal: Show how to use LLVM TableGen to model an Intel 8008–style target and consume the generated data from C++ in
  this repo.
- Scope: We generate RegisterInfo, InstrInfo, AsmWriter, and Disassembler tables. The demo executable prints the 8008
  register set and instruction names. We also add minimal encodings for a handful of control-flow instructions.
- Outcome: A small “backend-style” helper library and CLI integrated with LLVM 17. The design keeps
  dependencies light while demonstrating core TableGen workflows.

**Intel 8008 Overview**
- Data width: 8-bit accumulator architecture.
- Address space: 14-bit (up to ~16 KiB). We approximate with 16-bit immediates/PC for simplicity.
- Registers: Accumulator A; general registers B, C, D, E, H, L; register pairs BC, DE, HL used for addressing/data.
- Control flow: CALL/RET use an internal return stack; conditional jumps include JZ/JNZ. We model `JMP`, `JZ`, `JNZ`,
  `CALL`, `RET`, `HLT` mnemonics.
- Flags: Status flags (Z, C, S, P, AC). We model them as a single FLAGS register and mark instructions that define or
  use FLAGS as appropriate.

**Our Approximations and Semantics**
- PC width: PC and address immediates are modeled as `i16` rather than a dedicated 14-bit type. Range checks could be
  enforced later in parsers/encoders.
- Register pairs: `BC`, `DE`, `HL` are standalone 16-bit registers (no subregister indices). This avoids
  `RegisterWithSubRegs` complexity and keeps the model simple.
- Flags: `FLAGS` is a single 8-bit register. Arithmetic/logic ops set `Defs = [FLAGS]`. Conditional branches set
  `Uses = [FLAGS]`.
- Return stack: We approximate a 7-level pushdown stack with `RSP` (stack index) and `RSTK0..RSTK6` (16-bit return
  slots). `CALL` and `RET` mark `Uses/Defs` on `RSP` to indicate index changes.
- Register classes: `R8` = `[A,B,C,D,E,H,L]`, `R16` = `[BC,DE,HL,PC]`. Additional classes group `FLAGS`, `RSP`, and the
  `RSTK` slots.
- Instructions and encodings: Encodings are provided for a few control-flow instructions via a simple `Enc8` mixin and
  8-bit opcode field. Other instructions remain illustrative without encodings.

**Project Structure**
- TableGen target: `backend/intel8008/Intel8008.td`
  - Target anchor: `def Intel8008 : Target { let InstructionSet = Intel8008InstrInfo; }`.
  - Registers: `A, B, C, D, E, H, L, BC, DE, HL, PC, FLAGS, RSP, RSTK0..RSTK6`.
  - Register classes: `R8`, `R16`, plus classes for FLAGS, RSP, and RSTK slots.
  - Instruction subset: `MOVrr`, `MVIri`, `ADDr`, `SUBr`, `ANIi`, `JMPi`, `JZ_i`, `JNZ_i`, `CALLi`, `RET`, `HLT`.
  - Minimal encodings for: `JMPi`, `JZ_i`, `JNZ_i`, `CALLi`, `RET`, `HLT`.
- Generated files (build tree):
  - `Intel8008GenRegisterInfo.inc` via `-gen-register-info`.
  - `Intel8008GenInstrInfo.inc` via `-gen-instr-info`.
  - `Intel8008GenAsmWriter.inc` via `-gen-asm-writer`.
  - `Intel8008GenDisassemblerTables.inc` via `-gen-disassembler`.
- Backend glue (minimal):
  - Library: `intel8008_backend`.
  - C++: `backend/intel8008/Intel8008MCTargetDesc.h`, `.cpp`.
  - CLI: `src/intel8008_info.cpp` prints registers and instruction names.

**Build and Run**
- Prerequisites: LLVM 17 development files configured via the top-level CMake.
- Configure and build:
  - `cmake -S . -B build -G Ninja`
  - `cmake --build build -j`
- Run the demo:
  - `./build/intel8008-info`
  - Output lists 20 registers (includes FLAGS, RSP, and RSTK0..RSTK6) and the instruction set (generic + our ops).

**Generation Pipeline and Integration**
- TableGen invocations in CMake:
  - `-gen-register-info`, `-gen-instr-info`, `-gen-asm-writer`, `-gen-disassembler` with `-I ${LLVM_INCLUDE_DIRS}`.
- C++ consumption:
  - Includes `GET_INSTRINFO_ENUM` and `GET_INSTRINFO_MC_DESC` to initialize `MCInstrInfo` and access instruction names.
  - Bridges the register class ID enum names expected by the generated InstrInfo into the `llvm::Intel8008` namespace
    to satisfy references.
  - For now, the AsmWriter/Disassembler tables are generated but not exercised by a custom `MCInstPrinter` or
    `MCDisassembler` class. Those can be added later if needed.

**Technical Details**
- Instruction helper: `class I<dag outs, dag ins, string asmstr>` with `let Namespace = "Intel8008";` produces target-
  namespaced opcodes and asm strings.
- Encoding helper: `class Enc8 { field bits<8> Inst; }` used to attach 8-bit opcodes and `Size` to a few instructions.
- Flags and stack semantics: carried via `let Defs`/`let Uses` lists to signal side effects and dependencies.
- Simplicity: No `RegisterWithSubRegs`, no subtarget features or itineraries, and only a small instruction subset.

**How to Extend**
- Enforce 14-bit PC: add a custom `i14imm` operand with parser/decoder checks; mask/truncate encodings as necessary.
- Add more encodings: wire `MOV/MVI/ADD/SUB/ANI` encodings so they appear in disassembler tables and can round-trip.
- Implement printer/disassembler: create a minimal `MCInstPrinter` that includes `Intel8008GenAsmWriter.inc` and a
  small `MCDisassembler` using `Intel8008GenDisassemblerTables.inc` for real asm/bytes round-trips.
- Add subregister modeling: if needed for pair semantics, introduce `RegisterWithSubRegs` and explicit subreg indices.
- Subtarget and scheduling: add features, scheduling, and eventually ISel if you want codegen from LLVM IR.

**Tradeoffs and Rationale**
- Build-friendly: Focus on what’s needed to demo TableGen to C++ without the full complexity of a production backend.
- Faithfulness vs. clarity: The 14-bit PC and hardware stack are approximated in a way that remains simple and
  educational, while highlighting how you would evolve toward a complete target.
