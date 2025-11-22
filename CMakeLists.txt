cmake_minimum_required(VERSION 3.24)

# Strictly require LLVM 17; auto-detect it and configure toolchain from that
if(NOT DEFINED LLVM_DIR)
  # Prefer llvm-config-17
  find_program(LLVM_CONFIG_17 NAMES llvm-config-17 HINTS /opt/homebrew/opt/llvm@17/bin /usr/local/opt/llvm@17/bin /usr/lib/llvm-17/bin)
  if(LLVM_CONFIG_17)
    execute_process(COMMAND "${LLVM_CONFIG_17}" --prefix OUTPUT_VARIABLE _llvm_prefix OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(LLVM_DIR "${_llvm_prefix}/lib/cmake/llvm" CACHE PATH "LLVM 17 cmake dir" FORCE)
  else()
    # Try Homebrew default path
    if(EXISTS "/opt/homebrew/opt/llvm@17/lib/cmake/llvm/LLVMConfig.cmake")
      set(LLVM_DIR "/opt/homebrew/opt/llvm@17/lib/cmake/llvm" CACHE PATH "LLVM 17 cmake dir" FORCE)
    endif()
  endif()
endif()

if(NOT DEFINED LLVM_DIR OR NOT EXISTS "${LLVM_DIR}/LLVMConfig.cmake")
  message(FATAL_ERROR "LLVM 17 not found. Please install llvm@17 and set LLVM_DIR to <prefix>/lib/cmake/llvm.\nExamples:\n  macOS (Homebrew): brew install llvm@17 && export LLVM_DIR=\"\$(brew --prefix llvm@17)/lib/cmake/llvm\"\n  Linux: install llvm-17-dev and set LLVM_DIR accordingly.")
endif()

# Note: We intentionally do not override C or C++ compilers here.
# We only enforce that the LLVM package resolved below is version 17.

# Fakelang: a tiny OO language -> LLVM IR (LLVM 17)
project(fakelang-llvm LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FAKELANG_BUILD_TESTS "Build unit/integration/e2e tests" ON)

# No fallback: enforce LLVM 17 toolchain only

find_package(LLVM 17 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_definitions(${LLVM_DEFINITIONS})

add_library(fakelang STATIC
  src/Token.h
  src/Lexer.h
  src/Lexer.cpp
  src/AST.h
  src/Parser.h
  src/Parser.cpp
  src/CodeGen.h
  src/CodeGen.cpp
  src/IRAnnotator.h
  src/IRAnnotator.cpp
)

target_include_directories(fakelang PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(fakelang SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})

target_compile_options(fakelang PRIVATE
  -Wall -Wextra -Wpedantic -Wshadow -Wformat=2
)

target_link_libraries(fakelang PRIVATE
  LLVMCore
  LLVMSupport
)

add_executable(fakelangc src/main.cpp)
target_link_libraries(fakelangc PRIVATE fakelang)
target_include_directories(fakelangc SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})

if(FAKELANG_BUILD_TESTS)
  include(CTest)
  enable_testing()

  include(FetchContent)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  add_executable(fakelang_tests
    tests/LexerTests.cpp
    tests/ParserTests.cpp
    tests/CodeGenTests.cpp
    tests/E2EExampleTest.cpp
  )
  target_link_libraries(fakelang_tests PRIVATE fakelang GTest::gtest_main)
  target_include_directories(fakelang_tests SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
  # Optionally reduce noise from LLVM headers when using C++23
  option(FAKELANG_SUPPRESS_LLVM_WARNINGS "Suppress LLVM header deprecation warnings" ON)
  if(FAKELANG_SUPPRESS_LLVM_WARNINGS)
    target_compile_options(fakelang PRIVATE -Wno-deprecated-declarations)
    target_compile_options(fakelangc PRIVATE -Wno-deprecated-declarations)
    target_compile_options(fakelang_tests PRIVATE -Wno-deprecated-declarations)
  endif()
  target_compile_definitions(fakelang_tests PRIVATE FAKELANG_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  include(GoogleTest)
  gtest_discover_tests(fakelang_tests)
endif()

# ----------------------------------------------------------------------------
# Demo Intel8008 backend (out-of-tree style) using TableGen
# ----------------------------------------------------------------------------

find_program(LLVM_TBLGEN NAMES llvm-tblgen HINTS ${LLVM_TOOLS_BINARY_DIR})
if(NOT LLVM_TBLGEN)
  message(FATAL_ERROR "llvm-tblgen not found. Install LLVM tools (any recent version).")
endif()

set(INTEL8008_TD ${CMAKE_CURRENT_SOURCE_DIR}/backend/intel8008/Intel8008.td)
set(INTEL8008_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/intel8008)
file(MAKE_DIRECTORY ${INTEL8008_GEN_DIR})

set(INTEL8008_GEN_HEADERS
  ${INTEL8008_GEN_DIR}/Intel8008GenRegisterInfo.inc
  ${INTEL8008_GEN_DIR}/Intel8008GenInstrInfo.inc
  ${INTEL8008_GEN_DIR}/Intel8008GenAsmWriter.inc
  ${INTEL8008_GEN_DIR}/Intel8008GenDisassemblerTables.inc
)

add_custom_command(
  OUTPUT ${INTEL8008_GEN_DIR}/Intel8008GenRegisterInfo.inc
  COMMAND ${LLVM_TBLGEN} -I ${LLVM_INCLUDE_DIRS}
          -gen-register-info -o ${INTEL8008_GEN_DIR}/Intel8008GenRegisterInfo.inc
          ${INTEL8008_TD}
  DEPENDS ${INTEL8008_TD}
  COMMENT "TableGen: Generating Intel8008GenRegisterInfo.inc"
  VERBATIM)

add_custom_command(
  OUTPUT ${INTEL8008_GEN_DIR}/Intel8008GenInstrInfo.inc
  COMMAND ${LLVM_TBLGEN} -I ${LLVM_INCLUDE_DIRS}
          -gen-instr-info -o ${INTEL8008_GEN_DIR}/Intel8008GenInstrInfo.inc
          ${INTEL8008_TD}
  DEPENDS ${INTEL8008_TD}
  COMMENT "TableGen: Generating Intel8008GenInstrInfo.inc"
  VERBATIM)

add_custom_command(
  OUTPUT ${INTEL8008_GEN_DIR}/Intel8008GenAsmWriter.inc
  COMMAND ${LLVM_TBLGEN} -I ${LLVM_INCLUDE_DIRS}
          -gen-asm-writer -o ${INTEL8008_GEN_DIR}/Intel8008GenAsmWriter.inc
          ${INTEL8008_TD}
  DEPENDS ${INTEL8008_TD}
  COMMENT "TableGen: Generating Intel8008GenAsmWriter.inc"
  VERBATIM)

add_custom_command(
  OUTPUT ${INTEL8008_GEN_DIR}/Intel8008GenDisassemblerTables.inc
  COMMAND ${LLVM_TBLGEN} -I ${LLVM_INCLUDE_DIRS}
          -gen-disassembler -o ${INTEL8008_GEN_DIR}/Intel8008GenDisassemblerTables.inc
          ${INTEL8008_TD}
  DEPENDS ${INTEL8008_TD}
  COMMENT "TableGen: Generating Intel8008GenDisassemblerTables.inc"
  VERBATIM)


add_custom_target(intel8008_tablegen DEPENDS ${INTEL8008_GEN_HEADERS})

add_library(intel8008_backend STATIC
  backend/intel8008/Intel8008MCTargetDesc.h
  backend/intel8008/Intel8008MCTargetDesc.cpp
  ${INTEL8008_GEN_HEADERS}
)
add_dependencies(intel8008_backend intel8008_tablegen)
target_include_directories(intel8008_backend PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/backend/intel8008
  ${INTEL8008_GEN_DIR}
)
target_include_directories(intel8008_backend SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})
target_link_libraries(intel8008_backend PRIVATE LLVMSupport LLVMMC)
target_compile_options(intel8008_backend PRIVATE -Wall -Wextra -Wpedantic)

add_executable(intel8008-info src/intel8008_info.cpp)
target_link_libraries(intel8008-info PRIVATE intel8008_backend)
target_include_directories(intel8008-info SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
