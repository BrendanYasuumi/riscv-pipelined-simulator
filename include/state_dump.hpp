#pragma once

#include <cstdint>
#include <iosfwd>
#include <vector>

#include "cpu.hpp"

namespace rv32i {

struct MemoryDumpRange {
    uint32_t start_address = 0;
    uint32_t byte_count = 0;
};

void print_register_dump(std::ostream& output, const CPU& cpu);
void print_memory_dump(std::ostream& output,
                       const CPU& cpu,
                       uint32_t start_address,
                       uint32_t byte_count);
void print_written_memory_dump(std::ostream& output, const CPU& cpu);
void print_architectural_state_json(
    std::ostream& output,
    const CPU& cpu,
    const std::vector<MemoryDumpRange>& memory_ranges = {});

}  // namespace rv32i
