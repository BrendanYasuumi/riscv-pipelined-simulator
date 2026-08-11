#include <array>
#include <cstdint>
#include <cstdlib>
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
#include "stages.hpp"

namespace {

struct DemoInstruction {
    uint32_t encoding;
    std::string_view assembly;
};

struct CliOptions {
    rv32i::Config config{};
    std::string program_path;
    uint64_t max_cycles = 100;
    uint64_t retire_count = 0;
    bool has_retire_count = false;
    bool trace = false;
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
        throw std::runtime_error("invalid value for " + std::string(option_name));
    }
}

rv32i::BranchPredictorType parse_branch_predictor(std::string_view value) {
    if (value == "always-not-taken") {
        return rv32i::BranchPredictorType::AlwaysNotTaken;
    }
    if (value == "always-taken") {
        return rv32i::BranchPredictorType::AlwaysTaken;
    }
    if (value == "two-bit") {
        return rv32i::BranchPredictorType::TwoBitSaturatingCounter;
    }

    throw std::runtime_error("invalid branch predictor type");
}

CliOptions parse_cli(int argc, char* argv[]) {
    CliOptions options{};

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            options.show_help = true;
        } else if (arg == "--trace") {
            options.trace = true;
        } else if (arg == "--no-forwarding") {
            options.config.enable_forwarding = false;
        } else if (arg.rfind("--memory-latency=", 0) == 0) {
            const uint64_t value =
                parse_u64(arg.substr(17), "--memory-latency");
            if (value == 0) {
                throw std::runtime_error("memory latency must be at least 1");
            }
            options.config.memory_latency_cycles =
                static_cast<uint32_t>(value);
        } else if (arg.rfind("--max-cycles=", 0) == 0) {
            options.max_cycles = parse_u64(arg.substr(13), "--max-cycles");
        } else if (arg.rfind("--retire-count=", 0) == 0) {
            options.retire_count = parse_u64(arg.substr(15), "--retire-count");
            options.has_retire_count = true;
        } else if (arg.rfind("--branch-predictor=", 0) == 0) {
            options.config.branch_predictor_type =
                parse_branch_predictor(arg.substr(19));
        } else if (!arg.empty() && arg[0] == '-') {
            throw std::runtime_error("unknown option: " + std::string(arg));
        } else if (options.program_path.empty()) {
            options.program_path = std::string(arg);
        } else {
            throw std::runtime_error("only one program path may be provided");
        }
    }

    return options;
}

const char* branch_predictor_name(rv32i::BranchPredictorType type) {
    switch (type) {
        case rv32i::BranchPredictorType::AlwaysNotTaken:
            return "always-not-taken";
        case rv32i::BranchPredictorType::AlwaysTaken:
            return "always-taken";
        case rv32i::BranchPredictorType::TwoBitSaturatingCounter:
            return "two-bit";
    }

    return "unknown";
}

void print_usage(const char* executable) {
    std::cout << "Usage: " << executable << " [program.hex] [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --no-forwarding\n";
    std::cout << "  --memory-latency=N\n";
    std::cout << "  --branch-predictor=always-not-taken|always-taken|two-bit\n";
    std::cout << "  --max-cycles=N\n";
    std::cout << "  --retire-count=N\n";
    std::cout << "  --trace\n";
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

void print_config(const rv32i::Config& config, uint64_t max_cycles) {
    std::cout << "\nConfiguration:\n";
    std::cout << "  Forwarding:       "
              << (config.enable_forwarding ? "enabled" : "disabled") << '\n';
    std::cout << "  Memory latency:   " << config.memory_latency_cycles
              << " cycle(s)\n";
    std::cout << "  Branch predictor: "
              << branch_predictor_name(config.branch_predictor_type) << '\n';
    std::cout << "  Max cycles:       " << max_cycles << '\n';
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
    std::cout << "  Branch mispredictions: " << stats.branch_mispredictions
              << '\n';
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
            program = rv32i::load_hex_program(options.program_path);
            std::cout << "Loaded hex program: " << options.program_path
                      << " (" << program.instruction_count
                      << " instruction(s))\n";
        }

        rv32i::CPU cpu(4096, options.config);
        rv32i::PipelineRegisters pipeline{};
        cpu.load_program(program.bytes);

        print_config(cpu.config(), options.max_cycles);

        const uint64_t target_retired = options.has_retire_count
                                            ? options.retire_count
                                            : program.instruction_count;

        while (cpu.stats().instruction_count < target_retired &&
               cpu.stats().clock_cycles < options.max_cycles) {
            if (options.trace) {
                rv32i::print_pipeline_trace(std::cout, cpu, pipeline);
            }
            rv32i::run_pipeline_cycle(cpu, pipeline);
        }

        print_registers(cpu);
        print_stats(cpu);

        if (cpu.stats().instruction_count < target_retired) {
            std::cerr << "\nSimulation stopped before all instructions retired\n";
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        std::cerr << "Run with --help for usage.\n";
        return EXIT_FAILURE;
    }
}
