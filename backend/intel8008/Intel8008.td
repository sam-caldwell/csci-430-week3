//===- Intel8008.td - Top-level target description --------*- tablegen -*-===//
// A tiny, out-of-tree demonstration target approximating the Intel 8008.
// Defines the target, subtarget, registers, and a small instruction set.
// This is NOT a full codegen backend; it focuses on MC-level tables
// (registers/instructions) that we can consume from C++.
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

// The instruction set anchor used by Target definition below.
def Intel8008InstrInfo : InstrInfo;

// The LLVM target itself.
def Intel8008 : Target {
  let InstructionSet = Intel8008InstrInfo;
}

//------------------------------------------------------------------------------
// Registers
//------------------------------------------------------------------------------

// 8-bit general registers and pseudo 16-bit pairs (approximation).
class I8Reg<string n> : Register<n>;
class I16Reg<string n> : Register<n>;

def A  : I8Reg<"A">;
def B  : I8Reg<"B">;
def C  : I8Reg<"C">;
def D  : I8Reg<"D">;
def E  : I8Reg<"E">;
def H  : I8Reg<"H">;
def L  : I8Reg<"L">;

// Pseudo 16-bit pairs used for addressing/data (not exact 8008 semantics).
def BC : I16Reg<"BC">;
def DE : I16Reg<"DE">;
def HL : I16Reg<"HL">;

// Program counter (approximation; 8008 uses a 14-bit PC and a small stack).
def PC : I16Reg<"PC">;

// Flags and hardware return stack approximation.
// 8008 uses a small internal pushdown stack for return addresses.
// We approximate a 7-level stack and an index register RSP.
def FLAGS : I8Reg<"FLAGS">; // Z/C/S/P/AC flags packed (approximation)
def RSP   : I8Reg<"RSP">;   // Return stack index (0..6)
def RSTK0 : I16Reg<"RSTK0">;
def RSTK1 : I16Reg<"RSTK1">;
def RSTK2 : I16Reg<"RSTK2">;
def RSTK3 : I16Reg<"RSTK3">;
def RSTK4 : I16Reg<"RSTK4">;
def RSTK5 : I16Reg<"RSTK5">;
def RSTK6 : I16Reg<"RSTK6">;

// Register classes
let Namespace = "Intel8008" in {
  def R8  : RegisterClass<"Intel8008", [i8], 8, (add A, B, C, D, E, H, L)>;
  def R16 : RegisterClass<"Intel8008", [i16], 16, (add BC, DE, HL, PC)>;
  def FLAGSRC : RegisterClass<"Intel8008", [i8], 8, (add FLAGS)>;
  def RSPRC   : RegisterClass<"Intel8008", [i8], 8, (add RSP)>;
  def RSTKRC  : RegisterClass<"Intel8008", [i16], 16,
                              (add RSTK0, RSTK1, RSTK2, RSTK3, RSTK4, RSTK5, RSTK6)>;
}

//------------------------------------------------------------------------------
// Instructions (very small subset for demo)
//------------------------------------------------------------------------------

// A minimal instruction format helper.
class I<dag outs, dag ins, string asmstr, list<dag> pattern = []>
    : Instruction {
  let Namespace = "Intel8008";
  let OutOperandList = outs;
  let InOperandList  = ins;
  let AsmString = asmstr;
}

// Encoding helper for simple 8-bit opcode instructions.
class Enc8 {
  field bits<8> Inst;
}

// Data movement
def MOVrr : I<(outs R8:$dst), (ins R8:$src), "mov $dst, $src">;
def MVIri : I<(outs R8:$dst), (ins i8imm:$imm), "mvi $dst, $imm">;

// Arithmetic on accumulator A (simplified)
def ADDr  : I<(outs R8:$dst), (ins R8:$src), "add $src"> { let Defs = [FLAGS]; }
def SUBr  : I<(outs R8:$dst), (ins R8:$src), "sub $src"> { let Defs = [FLAGS]; }
def ANIi  : I<(outs R8:$dst), (ins i8imm:$imm), "ani $imm"> { let Defs = [FLAGS]; }

// Control flow (use 16-bit full addresses for the demo)
def JMPi  : I<(outs), (ins i16imm:$addr), "jmp $addr">,  Enc8 { let Inst = 0xC3; let Size = 3; let hasCompleteDecoder = 1; }
def JZ_i  : I<(outs), (ins i16imm:$addr), "jz $addr">,   Enc8 { let Inst = 0xCA; let Size = 3; let hasCompleteDecoder = 1; let Uses = [FLAGS]; }
def JNZ_i : I<(outs), (ins i16imm:$addr), "jnz $addr">,  Enc8 { let Inst = 0xC2; let Size = 3; let hasCompleteDecoder = 1; let Uses = [FLAGS]; }
def CALLi : I<(outs), (ins i16imm:$addr), "call $addr">, Enc8 { let Inst = 0xCD; let Size = 3; let hasCompleteDecoder = 1; let Uses = [RSP]; let Defs = [RSP]; }
def RET   : I<(outs), (ins),               "ret">,       Enc8 { let Inst = 0xC9; let Size = 1; let hasCompleteDecoder = 1; let Uses = [RSP]; let Defs = [RSP]; }
def HLT   : I<(outs), (ins),               "hlt">,       Enc8 { let Inst = 0x76; let Size = 1; let hasCompleteDecoder = 1; }
