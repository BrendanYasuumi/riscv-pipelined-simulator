#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cpu.hpp"
#include "state_dump.hpp"

namespace {

constexpr std::array<std::string_view, 32> kAbiRegisterNames{{
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0",   "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6",   "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6",
}};

struct Options {
    std::string input_path;
    std::string output_path;
    uint32_t halt_pc = 0;
    bool has_halt_pc = false;
    std::vector<rv32i::MemoryDumpRange> memory_ranges;
};

uint32_t parse_u32(std::string_view text, std::string_view name) {
    try {
        std::size_t parsed = 0;
        const uint64_t value = std::stoull(std::string(text), &parsed, 0);
        if (parsed != text.size() || value > UINT32_MAX) {
            throw std::invalid_argument("out of range");
        }
        return static_cast<uint32_t>(value);
    } catch (const std::exception&) {
        throw std::runtime_error("invalid value for " + std::string(name));
    }
}

rv32i::MemoryDumpRange parse_memory_range(std::string_view text) {
    const std::size_t separator = text.find(':');
    if (separator == std::string_view::npos) {
        throw std::runtime_error(
            "--memory-range must use START:LENGTH format");
    }

    const uint32_t start =
        parse_u32(text.substr(0, separator), "--memory-range start");
    const uint32_t length =
        parse_u32(text.substr(separator + 1), "--memory-range length");
    if ((start % 4) != 0 || length == 0 || (length % 4) != 0) {
        throw std::runtime_error(
            "Spike memory ranges must be nonempty and 4-byte aligned");
    }

    return {start, length};
}

Options parse_options(int argc, char* argv[]) {
    Options options{};

    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];
        if (argument.rfind("--input=", 0) == 0) {
            options.input_path = std::string(argument.substr(8));
        } else if (argument.rfind("--output=", 0) == 0) {
            options.output_path = std::string(argument.substr(9));
        } else if (argument.rfind("--halt-pc=", 0) == 0) {
            options.halt_pc = parse_u32(argument.substr(10), "--halt-pc");
            options.has_halt_pc = true;
        } else if (argument.rfind("--memory-range=", 0) == 0) {
            options.memory_ranges.push_back(
                parse_memory_range(argument.substr(15)));
        } else {
            throw std::runtime_error("unknown option: " +
                                     std::string(argument));
        }
    }

    if (options.input_path.empty() || options.output_path.empty() ||
        !options.has_halt_pc) {
        throw std::runtime_error(
            "required options: --input, --output, and --halt-pc");
    }

    return options;
}

std::string read_text_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open Spike output: " + path);
    }

    return {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
}

std::unordered_map<std::string, uint32_t> parse_registers(
    const std::string& text) {
    const std::regex register_pattern(
        R"(([a-z][a-z0-9]*):\s*(0x[0-9a-fA-F]+))");
    std::unordered_map<std::string, uint32_t> registers;

    for (std::sregex_iterator match(text.begin(), text.end(), register_pattern),
         end;
         match != end;
         ++match) {
        registers[(*match)[1].str()] =
            parse_u32((*match)[2].str(), "Spike register");
    }

    if (registers.size() != kAbiRegisterNames.size()) {
        throw std::runtime_error(
            "Spike output contained " + std::to_string(registers.size()) +
            " of 32 registers");
    }
    return registers;
}

std::vector<uint32_t> parse_standalone_hex_values(const std::string& text) {
    const std::regex value_pattern(
        R"(\s*(0x[0-9a-fA-F]+)\s*)");
    std::vector<uint32_t> values;
    std::istringstream lines(text);
    std::string line;

    while (std::getline(lines, line)) {
        std::smatch match;
        if (std::regex_match(line, match, value_pattern)) {
            values.push_back(parse_u32(match[1].str(), "Spike value"));
        }
    }

    return values;
}

std::size_t required_memory_size(
    const std::vector<rv32i::MemoryDumpRange>& ranges,
    uint32_t halt_pc) {
    uint64_t end = static_cast<uint64_t>(halt_pc) + 4;
    for (const rv32i::MemoryDumpRange& range : ranges) {
        end = std::max(end,
                       static_cast<uint64_t>(range.start_address) +
                           range.byte_count);
    }

    if (end > UINT32_MAX) {
        throw std::runtime_error("architectural state exceeds 32-bit memory");
    }
    return static_cast<std::size_t>(end);
}

void convert(const Options& options) {
    const std::string text = read_text_file(options.input_path);
    const auto registers = parse_registers(text);
    const std::vector<uint32_t> values = parse_standalone_hex_values(text);

    std::size_t expected_words = 0;
    for (const rv32i::MemoryDumpRange& range : options.memory_ranges) {
        expected_words += range.byte_count / 4;
    }
    if (values.size() != expected_words + 1) {
        throw std::runtime_error(
            "Spike output did not contain the expected PC and memory words");
    }
    if (values.front() != options.halt_pc) {
        throw std::runtime_error("Spike stopped at an unexpected PC");
    }

    rv32i::CPU cpu(required_memory_size(options.memory_ranges,
                                        options.halt_pc));
    for (uint8_t index = 0; index < kAbiRegisterNames.size(); ++index) {
        cpu.write_reg(index,
                      registers.at(std::string(kAbiRegisterNames[index])));
    }

    std::size_t value_index = 1;
    for (const rv32i::MemoryDumpRange& range : options.memory_ranges) {
        for (uint32_t offset = 0; offset < range.byte_count; offset += 4) {
            cpu.write_u32(range.start_address + offset,
                          values[value_index++]);
        }
    }

    // Spike stops before EBREAK. The simulator retires it and reports the
    // sequential PC, so normalize Spike to that shared test-harness contract.
    cpu.set_pc(options.halt_pc + 4);
    cpu.halt();

    std::ofstream output(options.output_path, std::ios::trunc);
    if (!output) {
        throw std::runtime_error("failed to open output file: " +
                                 options.output_path);
    }
    rv32i::print_architectural_state_json(
        output, cpu, options.memory_ranges);
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        convert(parse_options(argc, argv));
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
