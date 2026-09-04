#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "config.hpp"
#include "cpu.hpp"
#include "pipeline_registers.hpp"
#include "pipeline_trace.hpp"
#include "program_loader.hpp"
#include "state_dump.hpp"
#include "stages.hpp"

namespace {

struct DemoInstruction {
    uint32_t encoding;
    std::string_view assembly;
};

struct MemoryExpectation {
    uint32_t address = 0;
    uint32_t expected_value = 0;
};

struct CliOptions {
    rv32i::Config config{};
    std::string program_path;
    uint32_t load_address = 0;
    uint32_t memory_size = 64 * 1024;
    uint64_t max_cycles = 100;
    uint64_t retire_count = 0;
    bool has_retire_count = false;
    bool dump_regs = false;
    bool dump_memory = false;
    bool dump_written_memory = false;
    std::string dump_state_path;
    std::vector<rv32i::MemoryDumpRange> state_memory_ranges;
    uint32_t dump_memory_start = 0;
    uint32_t dump_memory_length = 0;
    std::vector<MemoryExpectation> memory_expectations;
    bool trace = false;
    uint64_t trace_start = 0;
    uint64_t trace_limit = 0;
    bool has_trace_limit = false;
    bool show_help = false;
};

constexpr std::array<DemoInstruction, 3> kDemoProgram{{
    {0x00500093u, "addi x1, x0, 5"},
    {0x00700113u, "addi x2, x0, 7"},
    {0x002081B3u, "add x3, x1, x2"},
}};

void append_word_little_endian(std::vector<uint8_t>& bytes, uint32_t word) {
    bytes.push_back(static_cast<uint8_t>(word & 0xFFu));
    bytes.push_back(static_cast<uint8_t>((word >> 8) & 0xFFu));
    bytes.push_back(static_cast<uint8_t>((word >> 16) & 0xFFu));
    bytes.push_back(static_cast<uint8_t>((word >> 24) & 0xFFu));
}

rv32i::LoadedProgram make_demo_program() {
    rv32i::LoadedProgram program{};

    for (const DemoInstruction& instruction : kDemoProgram) {
        append_word_little_endian(program.bytes, instruction.encoding);
        ++program.instruction_count;
    }

    return program;
}

uint64_t parse_u64(std::string_view text, std::string_view option_name) {
    try {
        std::size_t parsed = 0;
        const uint64_t value = std::stoull(std::string(text), &parsed, 10);
        if (parsed != text.size()) {
            throw std::invalid_argument("trailing characters");
        }
        return value;
    } catch (const std::exception&) {
        throw std::runtime_error("invalid value for " +
                                 std::string(option_name));
    }
}

uint64_t parse_u64_auto(std::string_view text, std::string_view option_name) {
    try {
        std::size_t parsed = 0;
        const uint64_t value = std::stoull(std::string(text), &parsed, 0);
        if (parsed != text.size()) {
            throw std::invalid_argument("trailing characters");
        }
        return value;
    } catch (const std::exception&) {
        throw std::runtime_error("invalid value for " +
                                 std::string(option_name));
    }
}

void parse_memory_dump_range(std::string_view value, CliOptions& options) {
    const std::size_t separator = value.find(':');
    if (separator == std::string_view::npos) {
        throw std::runtime_error(
            "--dump-memory must use START:LENGTH format");
    }

    const uint64_t start =
        parse_u64_auto(value.substr(0, separator), "--dump-memory start");
    const uint64_t length =
        parse_u64_auto(value.substr(separator + 1), "--dump-memory length");

    if (start > UINT32_MAX || length > UINT32_MAX) {
        throw std::runtime_error("--dump-memory values must fit in 32 bits");
    }

    options.dump_memory = true;
    options.dump_memory_start = static_cast<uint32_t>(start);
    options.dump_memory_length = static_cast<uint32_t>(length);
}

rv32i::MemoryDumpRange parse_state_memory_range(std::string_view value) {
    const std::size_t separator = value.find(':');
    if (separator == std::string_view::npos) {
        throw std::runtime_error(
            "--state-memory must use START:LENGTH format");
    }

    const uint64_t start =
        parse_u64_auto(value.substr(0, separator), "--state-memory start");
    const uint64_t length =
        parse_u64_auto(value.substr(separator + 1), "--state-memory length");

    if (start > UINT32_MAX || length > UINT32_MAX) {
        throw std::runtime_error("--state-memory values must fit in 32 bits");
    }

    return {static_cast<uint32_t>(start), static_cast<uint32_t>(length)};
}

void parse_memory_expectation(std::string_view value, CliOptions& options) {
    const std::size_t separator = value.find(':');
    if (separator == std::string_view::npos) {
        throw std::runtime_error(
            "--expect-memory must use ADDRESS:VALUE format");
    }

    const uint64_t address =
        parse_u64_auto(value.substr(0, separator), "--expect-memory address");
    const uint64_t expected =
        parse_u64_auto(value.substr(separator + 1), "--expect-memory value");

    if (address > UINT32_MAX - 3ull || expected > UINT32_MAX) {
        throw std::runtime_error(
            "--expect-memory values must fit in a 32-bit word access");
    }

    options.memory_expectations.push_back(
        {static_cast<uint32_t>(address), static_cast<uint32_t>(expected)});
}

CliOptions parse_cli(int argc, char* argv[]) {
    CliOptions options{};

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            options.show_help = true;
        } else if (arg == "--trace") {
            options.trace = true;
        } else if (arg.rfind("--trace-start=", 0) == 0) {
            options.trace_start = parse_u64(arg.substr(14), "--trace-start");
            options.trace = true;
        } else if (arg.rfind("--trace-limit=", 0) == 0) {
            options.trace_limit = parse_u64(arg.substr(14), "--trace-limit");
            options.has_trace_limit = true;
            options.trace = true;
        } else if (arg == "--dump-regs") {
            options.dump_regs = true;
        } else if (arg == "--dump-written-memory") {
            options.dump_written_memory = true;
        } else if (arg.rfind("--dump-state=", 0) == 0) {
            options.dump_state_path = std::string(arg.substr(13));
            if (options.dump_state_path.empty()) {
                throw std::runtime_error("--dump-state requires a file path");
            }
        } else if (arg.rfind("--state-memory=", 0) == 0) {
            options.state_memory_ranges.push_back(
                parse_state_memory_range(arg.substr(15)));
        } else if (arg.rfind("--dump-memory=", 0) == 0) {
            parse_memory_dump_range(arg.substr(14), options);
        } else if (arg.rfind("--expect-memory=", 0) == 0) {
            parse_memory_expectation(arg.substr(16), options);
        } else if (arg.rfind("--max-cycles=", 0) == 0) {
            options.max_cycles = parse_u64(arg.substr(13), "--max-cycles");
        } else if (arg.rfind("--load-address=", 0) == 0) {
            const uint64_t address =
                parse_u64_auto(arg.substr(15), "--load-address");
            if (address > UINT32_MAX) {
                throw std::runtime_error(
                    "--load-address must fit in 32 bits");
            }
            options.load_address = static_cast<uint32_t>(address);
        } else if (arg.rfind("--memory-size=", 0) == 0) {
            const uint64_t size =
                parse_u64_auto(arg.substr(14), "--memory-size");
            if (size == 0 || size > UINT32_MAX) {
                throw std::runtime_error(
                    "--memory-size must be between 1 and 2^32-1 bytes");
            }
            options.memory_size = static_cast<uint32_t>(size);
        } else if (arg.rfind("--retire-count=", 0) == 0) {
            options.retire_count = parse_u64(arg.substr(15), "--retire-count");
            options.has_retire_count = true;
        } else if (!arg.empty() && arg[0] == '-') {
            throw std::runtime_error("unknown option: " + std::string(arg));
        } else if (options.program_path.empty()) {
            options.program_path = std::string(arg);
        } else {
            throw std::runtime_error("only one program path may be provided");
        }
    }

    if (!options.state_memory_ranges.empty() &&
        options.dump_state_path.empty()) {
        throw std::runtime_error(
            "--state-memory requires --dump-state=FILE");
    }

    return options;
}

void print_usage(const char* executable) {
    std::cout << "Usage: " << executable
              << " [program.bin] [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --max-cycles=N\n";
    std::cout << "  --load-address=ADDRESS\n";
    std::cout << "  --memory-size=BYTES\n";
    std::cout << "  --retire-count=N\n";
    std::cout << "  --dump-regs\n";
    std::cout << "  --dump-memory=START:LENGTH\n";
    std::cout << "  --dump-written-memory\n";
    std::cout << "  --dump-state=FILE\n";
    std::cout << "  --state-memory=START:LENGTH (repeatable)\n";
    std::cout << "  --expect-memory=ADDRESS:VALUE\n";
    std::cout << "  --trace\n";
    std::cout << "  --trace-start=N\n";
    std::cout << "  --trace-limit=N\n";
    std::cout << "  --help\n";
}

void print_demo_program() {
    std::cout << "Loaded built-in demo program:\n";

    uint32_t address = 0;
    for (const DemoInstruction& instruction : kDemoProgram) {
        std::cout << "  0x" << std::hex << std::setw(8) << std::setfill('0')
                  << address << ": 0x" << std::setw(8) << instruction.encoding
                  << std::dec << std::setfill(' ') << "    "
                  << instruction.assembly << '\n';
        address += sizeof(uint32_t);
    }
}

void print_run_options(const CliOptions& options) {
    std::cout << "\nRun options:\n";
    std::cout << "  Max cycles:       " << options.max_cycles << '\n';
    std::cout << "  Load address:     0x" << std::hex << options.load_address
              << std::dec << '\n';
    std::cout << "  Memory size:      " << options.memory_size << " bytes\n";
}

bool should_print_trace(const CliOptions& options,
                        uint64_t current_cycle,
                        uint64_t trace_lines_printed) {
    if (!options.trace || current_cycle < options.trace_start) {
        return false;
    }

    return !options.has_trace_limit ||
           trace_lines_printed < options.trace_limit;
}

void print_registers(const rv32i::CPU& cpu) {
    std::cout << "\nArchitectural registers:\n";
    for (uint8_t reg = 0; reg < 8; ++reg) {
        std::cout << "  x" << static_cast<int>(reg) << " = "
                  << cpu.read_reg(reg) << '\n';
    }
}

void print_stats(const rv32i::CPU& cpu) {
    const rv32i::ExecutionStats& stats = cpu.stats();

    std::cout << "\nExecution stats:\n";
    std::cout << "  Total cycles:          " << stats.clock_cycles << '\n';
    std::cout << "  Instructions retired:  " << stats.instruction_count << '\n';
    std::cout << "  CPI:                   " << stats.cpi() << '\n';
    std::cout << "  IPC:                   " << stats.ipc() << '\n';
    std::cout << "  Stall cycles:          " << stats.stall_cycles << '\n';
    std::cout << "  Halted:                "
              << (cpu.halted() ? "yes" : "no") << '\n';
}

bool check_memory_expectations(const rv32i::CPU& cpu,
                               const std::vector<MemoryExpectation>& checks) {
    if (checks.empty()) {
        return true;
    }

    bool all_passed = true;
    std::cout << "\nMemory expectations:\n";

    for (const MemoryExpectation& check : checks) {
        const uint32_t actual = cpu.read_u32(check.address);
        const bool passed = actual == check.expected_value;
        all_passed = all_passed && passed;

        std::cout << "  [" << (passed ? "PASS" : "FAIL") << "] memory[0x"
                  << std::hex << std::setw(8) << std::setfill('0')
                  << check.address << "] expected 0x" << std::setw(8)
                  << check.expected_value << ", actual 0x" << std::setw(8)
                  << actual << std::dec << std::setfill(' ') << '\n';
    }

    return all_passed;
}

void write_architectural_state(const CliOptions& options,
                               const rv32i::CPU& cpu) {
    if (options.dump_state_path.empty()) {
        return;
    }

    std::ofstream output(options.dump_state_path, std::ios::trunc);
    if (!output) {
        throw std::runtime_error("failed to open architectural-state file: " +
                                 options.dump_state_path);
    }

    rv32i::print_architectural_state_json(
        output, cpu, options.state_memory_ranges);
    if (!output) {
        throw std::runtime_error("failed to write architectural-state file: " +
                                 options.dump_state_path);
    }

    std::cout << "\nArchitectural state: " << options.dump_state_path << '\n';
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        const CliOptions options = parse_cli(argc, argv);
        if (options.show_help) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        }

        rv32i::LoadedProgram program{};
        if (options.program_path.empty()) {
            program = make_demo_program();
            print_demo_program();
        } else {
            program = rv32i::load_binary_program(options.program_path);
            std::cout << "Loaded binary image: " << options.program_path
                      << " (" << program.bytes.size() << " byte(s), "
                      << program.instruction_count << " word(s))\n";
        }

        rv32i::CPU cpu(options.memory_size, options.config);
        rv32i::PipelineRegisters pipeline{};
        cpu.load_program(program.bytes, options.load_address);

        print_run_options(options);

        uint64_t trace_lines_printed = 0;
        while (!cpu.halted() &&
               (!options.has_retire_count ||
                cpu.stats().instruction_count < options.retire_count) &&
               cpu.stats().clock_cycles < options.max_cycles) {
            if (should_print_trace(options,
                                   cpu.stats().clock_cycles,
                                   trace_lines_printed)) {
                rv32i::print_pipeline_trace(std::cout, cpu, pipeline);
                ++trace_lines_printed;
            }
            rv32i::run_pipeline_cycle(cpu, pipeline);
        }

        print_registers(cpu);
        print_stats(cpu);

        if (options.dump_regs) {
            rv32i::print_register_dump(std::cout, cpu);
        }
        if (options.dump_memory) {
            rv32i::print_memory_dump(std::cout,
                                     cpu,
                                     options.dump_memory_start,
                                     options.dump_memory_length);
        }
        if (options.dump_written_memory) {
            rv32i::print_written_memory_dump(std::cout, cpu);
        }
        write_architectural_state(options, cpu);

        const bool memory_expectations_passed =
            check_memory_expectations(cpu, options.memory_expectations);

        const bool reached_target =
            options.has_retire_count &&
            cpu.stats().instruction_count >= options.retire_count;
        if (!cpu.halted() && !reached_target) {
            std::cerr << "\nSimulation stopped before all instructions retired\n";
            return EXIT_FAILURE;
        }
        if (!memory_expectations_passed) {
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        std::cerr << "Run with --help for usage.\n";
        return EXIT_FAILURE;
    }
}
