CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
TARGET := simulator
TEST_TARGET := simulator_tests
REGRESSION_TMP_DIR := /tmp/rv32i_regression_actual

CORE_SOURCES := \
	src/cpu.cpp \
	src/decoder.cpp \
	src/forwarding_unit.cpp \
	src/hazard_unit.cpp \
	src/instruction.cpp \
	src/pipeline_trace.cpp \
	src/program_loader.cpp \
	src/state_dump.cpp \
	src/stages.cpp

SOURCES := main.cpp $(CORE_SOURCES)
TEST_SOURCES := tests/simulator_tests.cpp $(CORE_SOURCES)

.PHONY: all run test regression clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

$(TEST_TARGET): $(TEST_SOURCES)
	$(CXX) $(CXXFLAGS) $(TEST_SOURCES) -o $(TEST_TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

regression: $(TARGET)
	mkdir -p $(REGRESSION_TMP_DIR)
	./$(TARGET) examples/add.hex --dump-regs > $(REGRESSION_TMP_DIR)/add.txt
	diff -u tests/golden/add.txt $(REGRESSION_TMP_DIR)/add.txt
	./$(TARGET) examples/store_load.hex --dump-regs --dump-memory=64:16 > $(REGRESSION_TMP_DIR)/store_load.txt
	diff -u tests/golden/store_load.txt $(REGRESSION_TMP_DIR)/store_load.txt
	./$(TARGET) examples/branch_predictor.hex --retire-count=2 --branch-predictor=always-taken --dump-regs > $(REGRESSION_TMP_DIR)/branch_predictor.txt
	diff -u tests/golden/branch_predictor.txt $(REGRESSION_TMP_DIR)/branch_predictor.txt
	./$(TARGET) examples/memory_latency.hex --memory-latency=4 --dump-regs > $(REGRESSION_TMP_DIR)/memory_latency.txt
	diff -u tests/golden/memory_latency.txt $(REGRESSION_TMP_DIR)/memory_latency.txt
	./$(TARGET) examples/required_subset.hex --dump-regs --dump-memory=128:4 > $(REGRESSION_TMP_DIR)/required_subset.txt
	diff -u tests/golden/required_subset.txt $(REGRESSION_TMP_DIR)/required_subset.txt

clean:
	rm -f $(TARGET) $(TEST_TARGET)
