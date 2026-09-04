CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
TARGET := simulator
TEST_TARGET := simulator_tests
SPIKE_ADAPTER := spike_state_adapter
ASM ?= asmFiles/store_word.s
SIM_ARGS ?= --max-cycles=100000
TRACE_ARGS ?= --trace --trace-limit=25
STATE_FILE ?= build/architectural-state.json
STATE_ARGS ?=

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
SPIKE_ADAPTER_SOURCES := tools/spike_state_adapter.cpp src/cpu.cpp src/state_dump.cpp

.PHONY: all run trace state examples test asm-test golden clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

run: $(TARGET)
	./scripts/run_asm.sh $(ASM) --dump-regs --dump-written-memory $(SIM_ARGS)

trace: $(TARGET)
	./scripts/run_asm.sh $(ASM) $(TRACE_ARGS) $(SIM_ARGS)

state: $(TARGET)
	./scripts/run_asm.sh $(ASM) --dump-state=$(STATE_FILE) $(STATE_ARGS) $(SIM_ARGS)

examples: $(TARGET)
	./scripts/run_examples.sh

$(TEST_TARGET): $(TEST_SOURCES)
	$(CXX) $(CXXFLAGS) $(TEST_SOURCES) -o $(TEST_TARGET)

$(SPIKE_ADAPTER): $(SPIKE_ADAPTER_SOURCES)
	$(CXX) $(CXXFLAGS) $(SPIKE_ADAPTER_SOURCES) -o $(SPIKE_ADAPTER)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

asm-test: $(TARGET)
	./scripts/asm_memory_tests.sh

golden: $(TARGET) $(SPIKE_ADAPTER)
	./scripts/run_spike_golden.sh

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(SPIKE_ADAPTER)
	rm -rf build
