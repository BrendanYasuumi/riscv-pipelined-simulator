CXX := c++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
TARGET := simulator
TEST_TARGET := simulator_tests

CORE_SOURCES := \
	src/cpu.cpp \
	src/decoder.cpp \
	src/forwarding_unit.cpp \
	src/hazard_unit.cpp \
	src/instruction.cpp \
	src/program_loader.cpp \
	src/stages.cpp

SOURCES := main.cpp $(CORE_SOURCES)
TEST_SOURCES := tests/simulator_tests.cpp $(CORE_SOURCES)

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

$(TEST_TARGET): $(TEST_SOURCES)
	$(CXX) $(CXXFLAGS) $(TEST_SOURCES) -o $(TEST_TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(TARGET) $(TEST_TARGET)
