#include "program_loader.hpp"

#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>

namespace rv32i {

namespace {

std::string trim(const std::string& input) {
    std::size_t begin = 0;
    while (begin < input.size() &&
           std::isspace(static_cast<unsigned char>(input[begin]))) {
        ++begin;
    }

    std::size_t end = input.size();
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }

    return input.substr(begin, end - begin);
}

void append_word_little_endian(std::vector<uint8_t>& bytes, uint32_t word) {
    bytes.push_back(static_cast<uint8_t>(word & 0xFFu));
    bytes.push_back(static_cast<uint8_t>((word >> 8) & 0xFFu));
    bytes.push_back(static_cast<uint8_t>((word >> 16) & 0xFFu));
    bytes.push_back(static_cast<uint8_t>((word >> 24) & 0xFFu));
}

}  // namespace

LoadedProgram load_hex_program(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open hex program: " + path);
    }

    LoadedProgram program{};
    std::string line;
    uint64_t line_number = 0;

    while (std::getline(input, line)) {
        ++line_number;

        const std::size_t comment = line.find('#');
        if (comment != std::string::npos) {
            line.erase(comment);
        }

        line = trim(line);
        if (line.empty()) {
            continue;
        }

        if (line.size() > 2 && line[0] == '0' &&
            (line[1] == 'x' || line[1] == 'X')) {
            line.erase(0, 2);
        }

        if (line.size() > 8) {
            throw std::runtime_error("hex word too wide at line " +
                                     std::to_string(line_number));
        }

        std::size_t parsed = 0;
        const uint32_t word =
            static_cast<uint32_t>(std::stoul(line, &parsed, 16));
        if (parsed != line.size()) {
            throw std::runtime_error("invalid hex word at line " +
                                     std::to_string(line_number));
        }

        append_word_little_endian(program.bytes, word);
        ++program.instruction_count;
    }

    if (program.instruction_count == 0) {
        throw std::runtime_error("hex program contains no instructions");
    }

    return program;
}

}  // namespace rv32i
