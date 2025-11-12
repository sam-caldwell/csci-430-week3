# Fakelang: A Tiny OO Language to LLVM IR (LLVM 17)

Fakelang is a deliberately small, instructional compiler written in C++23 that demonstrates how to lower an 
object-oriented language with classes, single inheritance, and virtual methods to LLVM IR. It’s designed for readers 
comfortable with modern C++ but new to compilers and LLVM.

## Evidence of Successful Run
See the [Demonstration Outputs](Evidence-Of-Build.md). which includes instructions for using fakelangc and outputs from
an actual run.

## Highlights:
- Classes, single inheritance, and virtual methods (vtable-based dispatch)
- Minimal standard library: just `print(<string>)` via `puts`
- Code generation using LLVM 17 IRBuilder with opaque pointers
- Clean, handwritten lexer and parser (no parser generators)
- GTest unit, integration, and e2e tests
- CMake + Ninja build


## Quick Start


### Prerequisites:
- LLVM 17 development packages installed (headers + libs)
- Clang 17+ (recommended) and CMake 3.24+
- Ninja (recommended generator)


### Configure and build:


#### System Setup
- macOS (Homebrew example):
  - `brew install llvm@17 ninja`
  - `export LLVM_DIR="$(brew --prefix llvm@17)/lib/cmake/llvm"`
- Linux: install LLVM 17 dev packages, then set `LLVM_DIR` accordingly.


#### Build the compiler
- `cmake -S . -B build -G Ninja -DLLVM_DIR=${LLVM_DIR}`
- `cmake --build build`
- `ctest --test-dir build --output-on-failure`


#### Run the compiler:
- `./build/fakelangc demo/example.fakelang -o -` (prints LLVM IR to stdout)
- `./build/fakelangc demo/example.fakelang -o demo/example.ll`


## The Fakelang Language

For the demo, the language includes:
  - `class C { ... }` and `class D extends C { ... }`
  - Methods with no parameters, optional modifiers: `virtual` and `override`
  - A single return type per method (`String` for the demo methods)
  - A `main` function returning `Int`
  - Statements: variable declarations, `print(expr)`, and `return expr;`
  - Expressions: string/int literals, `new Class()`, variable references, and `obj.method()`

See `demo/example.fakelang`:
```
class Animal {
  virtual speak(): String { return "Animal"; }
}
class Dog extends Animal {
  override speak(): String { return "Woof"; }
}
function main(): Int {
  var a: Animal = new Animal();
  var d: Animal = new Dog();
  print(a.speak());
  print(d.speak());
  return 0;
}
```

## How Codegen Works

- Each object is a struct with its first field a pointer to its class vtable.
- Each vtable is a struct of `i8*` function pointers (slots). The base class defines the initial layout, and derived 
  classes override entries in-place.
- A virtual call loads the receiver’s vptr, indexes the slot, bitcasts the pointer to the concrete function type, and 
  calls it with `this`.
- Strings are global constants created via `IRBuilder::CreateGlobalStringPtr`. `print(x)` just calls `puts(x)`.


## Example artifacts you will see in the IR:
- `%class.Animal = type { ptr }`
- `%vtable.Animal = type { ptr }`
- `@vtable.Dog = private constant %vtable.Dog { ptr @Dog.speak ... }`
- `define i8* @Animal.speak(ptr %this)` and `define i8* @Dog.speak(ptr %this)`
- `declare i32 @puts(ptr)`


## Project Layout

- `src/Token.h`, `src/Lexer.*`: tiny lexer
- `src/AST.h`: simple AST node hierarchy
- `src/Parser.*`: handwritten recursive-descent parser
- `src/CodeGen.*`: LLVM 17 IRBuilder lowering
- `src/main.cpp`: CLI driver (`fakelangc`)
- `demo/example.fakelang`: demo program
- `tests/*.cpp`: unit, integration, and e2e tests (GTest)


## Implementation Notes (Instructional)

- **Opaque pointers (LLVM 17)**: LLVM now uses opaque `ptr` instead of typed pointers. This makes object and function 
  pointer casting easier for a teaching example.
- **Vtables**: The generator computes a slot layout by copying the base layout and then appending new virtual methods 
  or overriding existing ones. Each class defines its own vtable type and global value. Slots are stored as `i8*` so we 
  can bitcast to/from function pointers cleanly.
- **Allocation**: For simplicity, `new Class()` is lowered to a stack allocation (`alloca`) in `main`. In a production 
  compiler you would emit heap allocation plus a constructor. For the demo it’s sufficient and keeps the IR compact
  and readable.
- **Semantics**: The parser enforces only superficial rules. The code generator throws on missing base classes, illegal 
  overrides, or unknown references. Error handling is intentionally straightforward.


## Future Plans for FakeLang!
### Roadmap
- Apply to Y-Combinator and promote this as the next Ruby/Rust fad language of 2025!
- Make horribly opinionated changes to the language like Golang and create automated "no" stamp for issues.
- Implement old technologies and create new acronyms for them, then move back to SF and start a new company.
- Secure a VC round and squander a few million dollars promoting Fakelang.
- Enjoy the limelight until the next big thing.

### Alternative
- Enjoy my quiet existence, graduate, make money, and avoid people.
- Save humanity from yet another fad language and rant about Ruby/Rust/etc.
