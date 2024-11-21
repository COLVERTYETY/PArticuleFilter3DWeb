GTEST_DIR = /usr/local
TESTS = tests.cpp
TEST_BIN = tests

# Compiler and flags
EMCC = emcc
CXX = g++
CXXFLAGS = -O2 -s MODULARIZE=1 -s EXPORT_NAME='FilterModule' \
           -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] \
           -lembind
GTEST_FLAGS = -std=c++17 -I$(GTEST_DIR)/include -L$(GTEST_DIR)/lib -pthread

# Source files
SRC = filter.cpp
HEADERS = filter.h
BINDINGS_SRC = bindings.cpp

# Output files
OUT_JS = filter.js
OUT_WASM = filter.wasm

# Build target
all: $(OUT_JS)

# Compile to WebAssembly
$(OUT_JS): $(SRC) $(BINDINGS_SRC) $(HEADERS)
	$(EMCC) $(SRC) $(BINDINGS_SRC) -o $(OUT_JS) $(CXXFLAGS)

# Compile and run Google Test
tests: $(TESTS) $(SRC) $(HEADERS)
	$(CXX) $(TESTS) $(SRC) $(GTEST_FLAGS) -lgtest -lgtest_main -o $(TEST_BIN)

run-tests: tests
	./$(TEST_BIN)

# Clean generated files
clean:
	rm -f $(OUT_JS) $(OUT_WASM) $(TEST_BIN)

.PHONY: all clean tests run-tests
