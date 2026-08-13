#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rv32i {

struct LoadedProgram {
    std::vector<uint8_t> bytes;
    uint64_t instruction_count = 0;
};

enum class ProgramFormat {
    Auto,
    Hex,
    Binary
};

LoadedProgram load_hex_program(const std::string& path);
LoadedProgram load_binary_program(const std::string& path);
LoadedProgram load_program(const std::string& path, ProgramFormat format);

}  // namespace rv32i
