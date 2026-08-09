#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rv32i {

struct LoadedProgram {
    std::vector<uint8_t> bytes;
    uint64_t instruction_count = 0;
};

LoadedProgram load_hex_program(const std::string& path);

}  // namespace rv32i
