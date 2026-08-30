#pragma once

#include <cstdint>
#include <iosfwd>

#include "cpu.hpp"

namespace rv32i {

void print_register_dump(std::ostream& output, const CPU& cpu);
void print_memory_dump(std::ostream& output,
                       const CPU& cpu,
                       uint32_t start_address,
                       uint32_t byte_count);
void print_written_memory_dump(std::ostream& output, const CPU& cpu);

}  // namespace rv32i
