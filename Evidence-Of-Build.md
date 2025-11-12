# How To Use Fakelang

This guide shows the exact commands used locally to configure, build, test, and run the tiny Fakelang compiler, plus 
a sample of the generated LLVM IR and program output.

> Note: Paths and LLVM version output on your machine may differ. The project requires LLVM 17. The transcript below 
> was captured before pinning to LLVM 17; for your build, install LLVM 17 and set `LLVM_DIR` accordingly.

## Configure and Build

Commands run:

```
cmake -S . -B build -G Ninja
```

Sample output:
```
-- Could NOT find LibEdit (missing: LibEdit_INCLUDE_DIRS LibEdit_LIBRARIES)
-- Found LLVM 21.1.0
-- Using LLVMConfig.cmake in: /opt/homebrew/opt/llvm/lib/cmake/llvm
-- Configuring done (0.3s)
-- Generating done (0.0s)
-- Build files have been written to: /Users/samcaldwell/git/champlain/csci-430-llvm-ir/build
```

Build:
```
cmake --build build
```

Sample output:
```
ninja: no work to do.
```

## Run Tests

```
ctest --test-dir build --output-on-failure
```

Sample output:
```
Test project /Users/samcaldwell/git/champlain/csci-430-llvm-ir/build
    Start 1: Lexer.BasicTokens
1/4 Test #1: Lexer.BasicTokens ................   Passed    0.01 sec
    Start 2: Parser.ClassAndFunction
2/4 Test #2: Parser.ClassAndFunction ..........   Passed    0.01 sec
    Start 3: CodeGen.EmitsVTablesAndMethods
3/4 Test #3: CodeGen.EmitsVTablesAndMethods ...   Passed    0.01 sec
    Start 4: E2E.DemoExampleCompilesToIR
4/4 Test #4: E2E.DemoExampleCompilesToIR ......   Passed    0.01 sec

100% tests passed, 0 tests failed out of 4
```

## Generate LLVM IR from the Demo Program

Emit IR to a file:
```
./build/fakelangc demo/example.fakelang -o demo/example.ll
```

Emit IR to stdout:
```
./build/fakelangc demo/example.fakelang -o -
```

Sample IR (truncated):
```
; ModuleID = 'demo/example.fakelang'
source_filename = "fakelang-module"

%vtable.Animal = type { ptr }
%vtable.Dog = type { ptr }
%class.Animal = type { ptr }
%class.Dog = type { ptr }

@0 = private unnamed_addr constant [7 x i8] c"Animal\00", align 1
@1 = private unnamed_addr constant [5 x i8] c"Woof\00", align 1
@vtable.Animal = private constant %vtable.Animal { ptr @Animal.speak }
@vtable.Dog = private constant %vtable.Dog { ptr @Dog.speak }

define ptr @Animal.speak(ptr %0) {
entry:
  ret ptr @0
}

define ptr @Dog.speak(ptr %0) {
entry:
  ret ptr @1
}

declare i32 @puts(ptr)

define i32 @main() {
entry:
  %a.addr = alloca ptr, align 8
  %Animal.obj = alloca %class.Animal, align 8
  %Animal.vptr.addr = getelementptr inbounds nuw %class.Animal, ptr %Animal.obj, i32 0, i32 0
  store ptr @vtable.Animal, ptr %Animal.vptr.addr, align 8
  store ptr %Animal.obj, ptr %a.addr, align 8
  %d.addr = alloca ptr, align 8
  %Dog.obj = alloca %class.Dog, align 8
  %Dog.vptr.addr = getelementptr inbounds nuw %class.Dog, ptr %Dog.obj, i32 0, i32 0
  store ptr @vtable.Dog, ptr %Dog.vptr.addr, align 8
  store ptr %Dog.obj, ptr %d.addr, align 8
  %a.val = load ptr, ptr %a.addr, align 8
  %Animal.vptr.addr1 = getelementptr inbounds nuw %class.Animal, ptr %a.val, i32 0, i32 0
  %Animal.vptr = load ptr, ptr %Animal.vptr.addr1, align 8
  %speak.slot.addr = getelementptr inbounds nuw %vtable.Animal, ptr %Animal.vptr, i32 0, i32 0
  %speak.slot = load ptr, ptr %speak.slot.addr, align 8
  %speak.call = call ptr %speak.slot(ptr %a.val)
  %0 = call i32 @puts(ptr %speak.call)
  ; ...
}
```

## Run the Generated Program

Use Clang to compile the IR to a native executable and run it:
```
/usr/bin/clang demo/example.ll -o demo/example
./demo/example
```

Sample output:
```
Animal
Woof
```

That’s it — you’ve compiled `example.fakelang` to LLVM IR, and then to a native executable demonstrating class 
inheritance and virtual dispatch.
