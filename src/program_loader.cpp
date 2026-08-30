#include "program_loader.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>

namespace rv32i {

LoadedProgram load_binary_program(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to open binary program: " + path);
    }

    LoadedProgram program{};
    program.bytes.assign(std::istreambuf_iterator<char>(input),
                         std::istreambuf_iterator<char>());

    if (program.bytes.empty()) {
        throw std::runtime_error("binary program contains no instructions");
    }
    if (program.bytes.size() % sizeof(uint32_t) != 0) {
        throw std::runtime_error(
            "binary program byte count must be a multiple of 4");
    }

    program.instruction_count = program.bytes.size() / sizeof(uint32_t);
    return program;
}

}  // namespace rv32i
