# Thin Makefile wrapper around CMake+Ninja
# Targets:
#   make configure  - configure CMake into $(BUILD_DIR)
#   make build      - build using CMake in $(BUILD_DIR)
#   make test       - run all tests via CTest in $(BUILD_DIR)
#   make clean      - remove $(BUILD_DIR)
#   make zip        - create repo zip excluding $(BUILD_DIR)/, .git/, cmake-build-*/

.PHONY: all configure build test clean zip demo
# Avoid parallelizing Makefile targets; Ninja handles build parallelism.
# Prevents races like `make clean configure build -j` removing $(BUILD_DIR)
# while CMake/ctest operate on it.
.NOTPARALLEL:

BUILD_DIR ?= build
GENERATOR ?= Ninja
# Optional: set from CLI e.g. `make configure BUILD_TYPE=Release`
BUILD_TYPE ?=
# Extra flags: `make configure CMAKE_FLAGS="-DFAKELANG_BUILD_TESTS=OFF"`
CMAKE_FLAGS ?=
# Extra build args: `make build BUILD_ARGS=-v`
BUILD_ARGS ?=
# Zip output name (override with `make zip ZIP_FILE=name.zip`)
ZIP_FILE ?= repo.zip

all: clean configure build demo

configure:
	@mkdir -p "$(BUILD_DIR)"
	@set -e; \
	# Auto-detect LLVM_DIR if not provided (prefer llvm-config-17, fallback to brew)
	if [ -z "$$LLVM_DIR" ]; then \
	  if command -v llvm-config-17 >/dev/null 2>&1; then \
	    export LLVM_DIR="$$(llvm-config-17 --prefix)/lib/cmake/llvm"; \
	  elif command -v brew >/dev/null 2>&1 && brew --prefix llvm@17 >/dev/null 2>&1; then \
	    export LLVM_DIR="$$(brew --prefix llvm@17)/lib/cmake/llvm"; \
	  fi; \
	fi; \
	extra=""; \
	if [ -n "$$LLVM_DIR" ]; then extra="$$extra -DLLVM_DIR=$$LLVM_DIR"; fi; \
	if [ -n "$(BUILD_TYPE)" ]; then extra="$$extra -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)"; fi; \
	cmake -S . -B "$(BUILD_DIR)" -G "$(GENERATOR)" $$extra $(CMAKE_FLAGS)

build:
	@[ -f "$(BUILD_DIR)/CMakeCache.txt" ] || $(MAKE) configure
	@cmake --build "$(BUILD_DIR)" -- $(BUILD_ARGS)

# Build (if needed) and run all tests
test: build
	@echo "Running tests in $(BUILD_DIR)"
	@ctest --test-dir "$(BUILD_DIR)" --output-on-failure

# Build the compiler and compile the demo source to LLVM IR
demo: build
	@echo "Compiling demo/example.fakelang -> demo/example.ll"
	@"$(BUILD_DIR)/fakelangc" demo/example.fakelang -o demo/example.ll

clean:
	@rm -rf "$(BUILD_DIR)"
	@rm -rf cmake-build-*

zip:
	@mkdir -p "$(BUILD_DIR)"; \
	 name="$(BUILD_DIR)/$(ZIP_FILE)"; \
	 echo "Creating $$name (excluding $(BUILD_DIR)/, .git/, cmake-build-*/)"; \
	 rm -f "$$name"; \
	 find . \
	   -path "./$(BUILD_DIR)" -prune -o \
	   -path "./$(ZIP_FILE)" -prune -o \
	   -path "./.git" -prune -o \
	   -path "./.idea" -prune -o \
	   -type d -name "cmake-build-*" -prune -o \
	   -type f -print | sed 's|^\./||' | zip -q "$$name" -@; \
	 echo "Wrote $$name"
