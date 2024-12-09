# Directories
GTEST_DIR = /usr/local

# Source and header files
SRC = filter.cpp
HEADERS = filter.h
BINDINGS_SRC = bindings.cpp
TESTS = tests.cpp

# Output files
OUT_JS = filter.js
OUT_WASM = filter.wasm
TEST_BIN = tests.bin 

# Compiler and flags
EMCC = emcc
CXX = g++
CXXFLAGS = -O2 -s MODULARIZE=1 -s EXPORT_NAME='FilterModule' \
           -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] \
           -lembind -msimd128
GTEST_FLAGS = -std=c++17 -I$(GTEST_DIR)/include -L$(GTEST_DIR)/lib -pthread

# Eigen include directory
EIGEN_DIR = /usr/local/include/eigen3/

# Visualization folder
VIZ_FOLDER = analyze/

# Build target
all: $(OUT_JS)

# Compile to WebAssembly
$(OUT_JS): $(SRC) $(BINDINGS_SRC) $(HEADERS)
	$(EMCC) $(SRC) $(BINDINGS_SRC) -I$(EIGEN_DIR) -o $(OUT_JS) $(CXXFLAGS) 

# Compile and run Google Test
tests: $(TESTS) $(SRC) $(HEADERS)
	$(CXX) $(TESTS) $(SRC) $(GTEST_FLAGS) -I$(EIGEN_DIR) -lgtest -lgtest_main -o $(TEST_BIN) -O2

run-tests: tests
	./$(TEST_BIN)

viz:
	cd $(VIZ_FOLDER) && for script in *.py; do \
		python3 $$script; \
	done

# Clean generated files
clean:
	rm -f $(OUT_JS) $(OUT_WASM) $(TEST_BIN)

.PHONY: all clean tests run-tests viz
