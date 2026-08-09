#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string_view>

#include "cpu.hpp"
#include "pipeline_registers.hpp"
#include "stages.hpp"

namespace {

struct DemoInstruction {
    uint32_t encoding;
    std::string_view assembly;
};

constexpr std::array<DemoInstruction, 3> kDemoProgram{{
    {0x00500093u, "addi x1, x0, 5"},
    {0x00700113u, "addi x2, x0, 7"},
    {0x002081B3u, "add x3, x1, x2"},
}};

void load_demo_program(rv32i::CPU& cpu) {
    uint32_t address = 0;

    for (const DemoInstruction& instruction : kDemoProgram) {
        cpu.write_u32(address, instruction.encoding);
        address += sizeof(uint32_t);
    }

    cpu.set_pc(0);
}

void print_demo_program() {
    std::cout << "Loaded demo program:\n";

    uint32_t address = 0;
    for (const DemoInstruction& instruction : kDemoProgram) {
        std::cout << "  0x" << std::hex << std::setw(8) << std::setfill('0')
                  << address << ": 0x" << std::setw(8) << instruction.encoding
                  << std::dec << std::setfill(' ') << "    "
                  << instruction.assembly << '\n';
        address += sizeof(uint32_t);
    }
}

void print_registers(const rv32i::CPU& cpu) {
    std::cout << "\nArchitectural registers:\n";
    std::cout << "  x1 = " << cpu.read_reg(1) << '\n';
    std::cout << "  x2 = " << cpu.read_reg(2) << '\n';
    std::cout << "  x3 = " << cpu.read_reg(3) << '\n';
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

int main() {
    rv32i::Config config{};
    rv32i::CPU cpu(1024, config);
    rv32i::PipelineRegisters pipeline{};

    load_demo_program(cpu);
    print_demo_program();

    // Three instructions plus four drain cycles plus two RAW stalls. The stalls
    // are inserted automatically by the hazard unit; no manual NOPs are needed.
    constexpr uint64_t kCyclesToRun = kDemoProgram.size() + 4 + 2;
    for (uint64_t cycle = 0; cycle < kCyclesToRun; ++cycle) {
        rv32i::run_pipeline_cycle(cpu, pipeline);
    }

    print_registers(cpu);
    print_stats(cpu);

    return 0;
}
