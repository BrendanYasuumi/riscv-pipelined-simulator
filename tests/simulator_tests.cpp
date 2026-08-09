#include <cassert>
#include <cstdint>
#include <fstream>
#include <string>

#include "cpu.hpp"
#include "decoder.hpp"
#include "instruction.hpp"
#include "pipeline_registers.hpp"
#include "program_loader.hpp"
#include "stages.hpp"

namespace {

void run_until_retired(rv32i::CPU& cpu,
                       rv32i::PipelineRegisters& pipeline,
                       uint64_t retired,
                       uint64_t max_cycles = 100) {
    while (cpu.stats().instruction_count < retired &&
           cpu.stats().clock_cycles < max_cycles) {
        rv32i::run_pipeline_cycle(cpu, pipeline);
    }
}

void test_x0_is_hardwired_to_zero() {
    rv32i::CPU cpu(64);
    cpu.write_reg(0, 123);
    assert(cpu.read_reg(0) == 0);
}

void test_little_endian_memory() {
    rv32i::CPU cpu(64);
    cpu.write_u32(0, 0x12345678u);
    assert(cpu.read_u8(0) == 0x78);
    assert(cpu.read_u8(1) == 0x56);
    assert(cpu.read_u8(2) == 0x34);
    assert(cpu.read_u8(3) == 0x12);
    assert(cpu.read_u32(0) == 0x12345678u);
}

void test_decoder_extracts_r_type_fields() {
    const auto fields = rv32i::decode_r_type(0x002081B3u);
    assert(fields.opcode == 0x33);
    assert(fields.rd == 3);
    assert(fields.rs1 == 1);
    assert(fields.rs2 == 2);
    assert(fields.funct3 == 0);
    assert(fields.funct7 == 0);
}

void test_instruction_classification() {
    const auto add = rv32i::decode_instruction(0x002081B3u);
    assert(add.kind == rv32i::InstructionKind::Add);
    assert(add.control.reg_write);
    assert(add.control.alu_op == rv32i::ALUOp::Add);
    assert(add.control.writeback_source == rv32i::WritebackSource::ALU);
}

void test_forwarding_removes_alu_stalls() {
    rv32i::Config config{};
    config.enable_forwarding = true;
    rv32i::CPU cpu(64, config);
    rv32i::PipelineRegisters pipeline;

    cpu.write_u32(0, 0x00500093u);  // addi x1, x0, 5
    cpu.write_u32(4, 0x00700113u);  // addi x2, x0, 7
    cpu.write_u32(8, 0x002081B3u);  // add x3, x1, x2

    run_until_retired(cpu, pipeline, 3);

    assert(cpu.read_reg(3) == 12);
    assert(cpu.stats().stall_cycles == 0);
}

void test_forwarding_disabled_stalls_on_alu_dependency() {
    rv32i::Config config{};
    config.enable_forwarding = false;
    rv32i::CPU cpu(64, config);
    rv32i::PipelineRegisters pipeline;

    cpu.write_u32(0, 0x00500093u);  // addi x1, x0, 5
    cpu.write_u32(4, 0x00700113u);  // addi x2, x0, 7
    cpu.write_u32(8, 0x002081B3u);  // add x3, x1, x2

    run_until_retired(cpu, pipeline, 3);

    assert(cpu.read_reg(3) == 12);
    assert(cpu.stats().stall_cycles == 2);
}

void test_load_use_still_stalls_once_with_forwarding() {
    rv32i::Config config{};
    config.enable_forwarding = true;
    rv32i::CPU cpu(64, config);
    rv32i::PipelineRegisters pipeline;

    cpu.write_u32(0, 0x01002083u);  // lw x1, 16(x0)
    cpu.write_u32(4, 0x00108133u);  // add x2, x1, x1
    cpu.write_u32(16, 21);

    run_until_retired(cpu, pipeline, 2);

    assert(cpu.read_reg(2) == 42);
    assert(cpu.stats().stall_cycles == 1);
}

void test_memory_latency_adds_stalls() {
    rv32i::Config config{};
    config.memory_latency_cycles = 3;
    rv32i::CPU cpu(64, config);
    rv32i::PipelineRegisters pipeline;

    cpu.write_u32(0, 0x01002083u);  // lw x1, 16(x0)
    cpu.write_u32(4, 0x00108133u);  // add x2, x1, x1
    cpu.write_u32(16, 21);

    run_until_retired(cpu, pipeline, 2);

    assert(cpu.read_reg(2) == 42);
    assert(cpu.stats().stall_cycles >= 3);
}

void test_hex_loader() {
    const std::string path = "/tmp/rv32i_loader_test.hex";
    {
        std::ofstream output(path);
        output << "# addi x1, x0, 5\n";
        output << "00500093\n";
        output << "0x00700113\n";
    }

    const rv32i::LoadedProgram program = rv32i::load_hex_program(path);
    assert(program.instruction_count == 2);
    assert(program.bytes.size() == 8);
    assert(program.bytes[0] == 0x93);
    assert(program.bytes[1] == 0x00);
    assert(program.bytes[2] == 0x50);
    assert(program.bytes[3] == 0x00);
}

void test_always_taken_branch_prediction() {
    rv32i::Config config{};
    config.branch_predictor_type = rv32i::BranchPredictorType::AlwaysTaken;
    rv32i::CPU cpu(64, config);
    rv32i::PipelineRegisters pipeline;

    cpu.write_u32(0, 0x00000463u);  // beq x0, x0, +8
    cpu.write_u32(4, 0x00100093u);  // addi x1, x0, 1
    cpu.write_u32(8, 0x00200093u);  // addi x1, x0, 2

    run_until_retired(cpu, pipeline, 2);

    assert(cpu.read_reg(1) == 2);
    assert(cpu.stats().branch_mispredictions == 0);
}

}  // namespace

int main() {
    test_x0_is_hardwired_to_zero();
    test_little_endian_memory();
    test_decoder_extracts_r_type_fields();
    test_instruction_classification();
    test_forwarding_removes_alu_stalls();
    test_forwarding_disabled_stalls_on_alu_dependency();
    test_load_use_still_stalls_once_with_forwarding();
    test_memory_latency_adds_stalls();
    test_hex_loader();
    test_always_taken_branch_prediction();
    return 0;
}
