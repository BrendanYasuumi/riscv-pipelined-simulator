#include <cassert>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "cpu.hpp"
#include "decoder.hpp"
#include "instruction.hpp"
#include "pipeline_registers.hpp"
#include "pipeline_trace.hpp"
#include "program_loader.hpp"
#include "stages.hpp"

namespace {

uint32_t encode_r(uint8_t funct7,
                  uint8_t rs2,
                  uint8_t rs1,
                  uint8_t funct3,
                  uint8_t rd) {
    return (static_cast<uint32_t>(funct7) << 25) |
           (static_cast<uint32_t>(rs2) << 20) |
           (static_cast<uint32_t>(rs1) << 15) |
           (static_cast<uint32_t>(funct3) << 12) |
           (static_cast<uint32_t>(rd) << 7) | 0x33u;
}

uint32_t encode_i(int32_t imm,
                  uint8_t rs1,
                  uint8_t funct3,
                  uint8_t rd,
                  uint8_t opcode = 0x13) {
    return ((static_cast<uint32_t>(imm) & 0xFFFu) << 20) |
           (static_cast<uint32_t>(rs1) << 15) |
           (static_cast<uint32_t>(funct3) << 12) |
           (static_cast<uint32_t>(rd) << 7) | opcode;
}

uint32_t encode_shift_i(uint8_t funct7,
                        uint8_t shamt,
                        uint8_t rs1,
                        uint8_t funct3,
                        uint8_t rd) {
    return (static_cast<uint32_t>(funct7) << 25) |
           (static_cast<uint32_t>(shamt & 0x1Fu) << 20) |
           (static_cast<uint32_t>(rs1) << 15) |
           (static_cast<uint32_t>(funct3) << 12) |
           (static_cast<uint32_t>(rd) << 7) | 0x13u;
}

uint32_t encode_s(int32_t imm, uint8_t rs2, uint8_t rs1, uint8_t funct3) {
    const uint32_t bits = static_cast<uint32_t>(imm) & 0xFFFu;
    return (((bits >> 5) & 0x7Fu) << 25) |
           (static_cast<uint32_t>(rs2) << 20) |
           (static_cast<uint32_t>(rs1) << 15) |
           (static_cast<uint32_t>(funct3) << 12) |
           ((bits & 0x1Fu) << 7) | 0x23u;
}

uint32_t encode_b(int32_t imm, uint8_t rs2, uint8_t rs1, uint8_t funct3) {
    const uint32_t bits = static_cast<uint32_t>(imm) & 0x1FFFu;
    return (((bits >> 12) & 0x1u) << 31) |
           (((bits >> 5) & 0x3Fu) << 25) |
           (static_cast<uint32_t>(rs2) << 20) |
           (static_cast<uint32_t>(rs1) << 15) |
           (static_cast<uint32_t>(funct3) << 12) |
           (((bits >> 1) & 0xFu) << 8) |
           (((bits >> 11) & 0x1u) << 7) | 0x63u;
}

uint32_t encode_u(uint32_t imm20, uint8_t rd, uint8_t opcode) {
    return ((imm20 & 0xFFFFFu) << 12) |
           (static_cast<uint32_t>(rd) << 7) | opcode;
}

uint32_t encode_j(int32_t imm, uint8_t rd) {
    const uint32_t bits = static_cast<uint32_t>(imm) & 0x1FFFFFu;
    return (((bits >> 20) & 0x1u) << 31) |
           (((bits >> 1) & 0x3FFu) << 21) |
           (((bits >> 11) & 0x1u) << 20) |
           (((bits >> 12) & 0xFFu) << 12) |
           (static_cast<uint32_t>(rd) << 7) | 0x6Fu;
}

void load_words(rv32i::CPU& cpu, const std::vector<uint32_t>& words) {
    uint32_t address = 0;
    for (uint32_t word : words) {
        cpu.write_u32(address, word);
        address += 4;
    }
    cpu.set_pc(0);
}

void run_until_retired(rv32i::CPU& cpu,
                       rv32i::PipelineRegisters& pipeline,
                       uint64_t retired,
                       uint64_t max_cycles = 100) {
    while (cpu.stats().instruction_count < retired &&
           cpu.stats().clock_cycles < max_cycles) {
        rv32i::run_pipeline_cycle(cpu, pipeline);
    }
}

rv32i::CPU run_program(const std::vector<uint32_t>& words,
                       uint64_t retired,
                       rv32i::Config config = {},
                       uint32_t memory_size = 256) {
    rv32i::CPU cpu(memory_size, config);
    rv32i::PipelineRegisters pipeline;
    load_words(cpu, words);
    run_until_retired(cpu, pipeline, retired);
    assert(cpu.stats().instruction_count == retired);
    return cpu;
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

void test_r_type_alu_execution() {
    const rv32i::CPU cpu = run_program({
        encode_i(12, 0, 0x0, 1),          // addi x1, x0, 12
        encode_i(5, 0, 0x0, 2),           // addi x2, x0, 5
        encode_r(0x00, 2, 1, 0x0, 3),     // add x3, x1, x2
        encode_r(0x20, 2, 1, 0x0, 4),     // sub x4, x1, x2
        encode_r(0x00, 2, 1, 0x7, 5),     // and x5, x1, x2
        encode_r(0x00, 2, 1, 0x6, 6),     // or x6, x1, x2
        encode_r(0x00, 2, 1, 0x4, 7),     // xor x7, x1, x2
        encode_r(0x00, 2, 1, 0x1, 8),     // sll x8, x1, x2
        encode_r(0x00, 2, 1, 0x5, 9),     // srl x9, x1, x2
        encode_i(-8, 0, 0x0, 10),         // addi x10, x0, -8
        encode_r(0x20, 2, 10, 0x5, 11),   // sra x11, x10, x2
        encode_r(0x00, 1, 2, 0x2, 12),    // slt x12, x2, x1
        encode_i(-1, 0, 0x0, 13),         // addi x13, x0, -1
        encode_r(0x00, 13, 2, 0x3, 14),   // sltu x14, x2, x13
    }, 14);

    assert(cpu.read_reg(3) == 17);
    assert(cpu.read_reg(4) == 7);
    assert(cpu.read_reg(5) == 4);
    assert(cpu.read_reg(6) == 13);
    assert(cpu.read_reg(7) == 9);
    assert(cpu.read_reg(8) == 384);
    assert(cpu.read_reg(9) == 0);
    assert(cpu.read_reg(11) == 0xFFFFFFFFu);
    assert(cpu.read_reg(12) == 1);
    assert(cpu.read_reg(14) == 1);
}

void test_i_type_alu_execution() {
    const rv32i::CPU cpu = run_program({
        encode_i(10, 0, 0x0, 1),          // addi x1, x0, 10
        encode_i(3, 1, 0x7, 2),           // andi x2, x1, 3
        encode_i(4, 1, 0x6, 3),           // ori x3, x1, 4
        encode_i(15, 1, 0x4, 4),          // xori x4, x1, 15
        encode_i(20, 1, 0x2, 5),          // slti x5, x1, 20
        encode_i(-1, 1, 0x3, 6),          // sltiu x6, x1, -1
        encode_shift_i(0x00, 2, 1, 0x1, 7),   // slli x7, x1, 2
        encode_shift_i(0x00, 1, 7, 0x5, 8),   // srli x8, x7, 1
        encode_i(-8, 0, 0x0, 9),          // addi x9, x0, -8
        encode_shift_i(0x20, 1, 9, 0x5, 10),  // srai x10, x9, 1
    }, 10);

    assert(cpu.read_reg(2) == 2);
    assert(cpu.read_reg(3) == 14);
    assert(cpu.read_reg(4) == 5);
    assert(cpu.read_reg(5) == 1);
    assert(cpu.read_reg(6) == 1);
    assert(cpu.read_reg(7) == 40);
    assert(cpu.read_reg(8) == 20);
    assert(cpu.read_reg(10) == 0xFFFFFFFCu);
}

void test_load_store_execution() {
    const rv32i::CPU cpu = run_program({
        encode_i(64, 0, 0x0, 1),          // addi x1, x0, 64
        encode_i(123, 0, 0x0, 2),         // addi x2, x0, 123
        encode_s(0, 2, 1, 0x2),           // sw x2, 0(x1)
        encode_i(0, 1, 0x2, 3, 0x03),     // lw x3, 0(x1)
        encode_i(-1, 0, 0x0, 4),          // addi x4, x0, -1
        encode_s(4, 4, 1, 0x0),           // sb x4, 4(x1)
        encode_i(4, 1, 0x0, 5, 0x03),     // lb x5, 4(x1)
        encode_i(4, 1, 0x4, 6, 0x03),     // lbu x6, 4(x1)
        encode_s(6, 4, 1, 0x1),           // sh x4, 6(x1)
        encode_i(6, 1, 0x1, 7, 0x03),     // lh x7, 6(x1)
        encode_i(6, 1, 0x5, 8, 0x03),     // lhu x8, 6(x1)
    }, 11);

    assert(cpu.read_reg(3) == 123);
    assert(cpu.read_reg(5) == 0xFFFFFFFFu);
    assert(cpu.read_reg(6) == 255);
    assert(cpu.read_reg(7) == 0xFFFFFFFFu);
    assert(cpu.read_reg(8) == 65535);
}

void test_lui_and_auipc_execution() {
    const rv32i::CPU cpu = run_program({
        encode_u(0x12345, 1, 0x37),       // lui x1, 0x12345
        encode_u(0x1, 2, 0x17),           // auipc x2, 0x1
    }, 2);

    assert(cpu.read_reg(1) == 0x12345000u);
    assert(cpu.read_reg(2) == 0x00001004u);
}

void test_branch_mispredict_flush_execution() {
    rv32i::Config config{};
    config.branch_predictor_type = rv32i::BranchPredictorType::AlwaysNotTaken;

    const rv32i::CPU cpu = run_program({
        encode_b(8, 0, 0, 0x0),           // beq x0, x0, +8
        encode_i(1, 0, 0x0, 1),           // addi x1, x0, 1
        encode_i(2, 0, 0x0, 1),           // addi x1, x0, 2
    }, 2, config);

    assert(cpu.read_reg(1) == 2);
    assert(cpu.stats().branch_mispredictions == 1);
}

void test_bne_execution_with_forwarded_operand() {
    const rv32i::CPU cpu = run_program({
        encode_i(1, 0, 0x0, 1),           // addi x1, x0, 1
        encode_b(8, 0, 1, 0x1),           // bne x1, x0, +8
        encode_i(11, 0, 0x0, 2),          // addi x2, x0, 11
        encode_i(22, 0, 0x0, 2),          // addi x2, x0, 22
    }, 3);

    assert(cpu.read_reg(2) == 22);
}

void test_jal_execution() {
    const rv32i::CPU cpu = run_program({
        encode_j(8, 1),                   // jal x1, +8
        encode_i(1, 0, 0x0, 2),           // addi x2, x0, 1
        encode_i(3, 0, 0x0, 2),           // addi x2, x0, 3
    }, 2);

    assert(cpu.read_reg(1) == 4);
    assert(cpu.read_reg(2) == 3);
}

void test_jalr_execution() {
    const rv32i::CPU cpu = run_program({
        encode_i(12, 0, 0x0, 5),          // addi x5, x0, 12
        encode_i(0, 5, 0x0, 1, 0x67),     // jalr x1, 0(x5)
        encode_i(1, 0, 0x0, 2),           // addi x2, x0, 1
        encode_i(2, 0, 0x0, 2),           // addi x2, x0, 2
    }, 3);

    assert(cpu.read_reg(1) == 8);
    assert(cpu.read_reg(2) == 2);
    assert(cpu.stats().branch_mispredictions >= 1);
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

void test_binary_loader() {
    const std::string path = "/tmp/rv32i_loader_test.bin";
    {
        std::ofstream output(path, std::ios::binary);
        const char bytes[] = {
            static_cast<char>(0x93),
            static_cast<char>(0x00),
            static_cast<char>(0x50),
            static_cast<char>(0x00),
            static_cast<char>(0x13),
            static_cast<char>(0x01),
            static_cast<char>(0x70),
            static_cast<char>(0x00),
        };
        output.write(bytes, sizeof(bytes));
    }

    const rv32i::LoadedProgram program = rv32i::load_binary_program(path);
    assert(program.instruction_count == 2);
    assert(program.bytes.size() == 8);
    assert(program.bytes[0] == 0x93);
    assert(program.bytes[4] == 0x13);
}

void test_auto_loader_infers_hex_and_binary() {
    const rv32i::LoadedProgram hex =
        rv32i::load_program("/tmp/rv32i_loader_test.hex",
                            rv32i::ProgramFormat::Auto);
    const rv32i::LoadedProgram bin =
        rv32i::load_program("/tmp/rv32i_loader_test.bin",
                            rv32i::ProgramFormat::Auto);

    assert(hex.instruction_count == 2);
    assert(bin.instruction_count == 2);
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

void test_always_taken_mispredicts_not_taken_branch() {
    rv32i::Config config{};
    config.branch_predictor_type = rv32i::BranchPredictorType::AlwaysTaken;

    const rv32i::CPU cpu = run_program({
        encode_b(8, 0, 0, 0x1),           // bne x0, x0, +8, not taken
        encode_i(1, 0, 0x0, 2),           // addi x2, x0, 1
        encode_i(2, 0, 0x0, 3),           // addi x3, x0, 2
    }, 3, config);

    assert(cpu.read_reg(2) == 1);
    assert(cpu.read_reg(3) == 2);
    assert(cpu.stats().branch_mispredictions == 1);
}

void test_two_bit_predictor_counter_updates() {
    rv32i::Config config{};
    config.branch_predictor_type =
        rv32i::BranchPredictorType::TwoBitSaturatingCounter;
    rv32i::CPU cpu(64, config);

    assert(!cpu.predict_branch(0));
    cpu.update_branch_predictor(0, true);
    assert(cpu.predict_branch(0));
    cpu.update_branch_predictor(0, true);
    assert(cpu.predict_branch(0));
    cpu.update_branch_predictor(0, false);
    assert(cpu.predict_branch(0));
    cpu.update_branch_predictor(0, false);
    assert(!cpu.predict_branch(0));
}

void test_pipeline_trace_csv_output() {
    rv32i::CPU cpu(64);
    rv32i::PipelineRegisters pipeline;
    std::ostringstream output;

    rv32i::write_pipeline_trace_csv_header(output);
    rv32i::write_pipeline_trace_csv_row(output, cpu, pipeline);

    const std::string csv = output.str();
    assert(csv.find("cycle,pc,if_id,id_ex,ex_mem,mem_wb") !=
           std::string::npos);
    assert(csv.find("0,0,bubble,bubble,bubble,bubble") !=
           std::string::npos);
}

}  // namespace

int main() {
    test_x0_is_hardwired_to_zero();
    test_little_endian_memory();
    test_decoder_extracts_r_type_fields();
    test_instruction_classification();
    test_r_type_alu_execution();
    test_i_type_alu_execution();
    test_load_store_execution();
    test_lui_and_auipc_execution();
    test_branch_mispredict_flush_execution();
    test_bne_execution_with_forwarded_operand();
    test_jal_execution();
    test_jalr_execution();
    test_forwarding_removes_alu_stalls();
    test_forwarding_disabled_stalls_on_alu_dependency();
    test_load_use_still_stalls_once_with_forwarding();
    test_memory_latency_adds_stalls();
    test_hex_loader();
    test_binary_loader();
    test_auto_loader_infers_hex_and_binary();
    test_always_taken_branch_prediction();
    test_always_taken_mispredicts_not_taken_branch();
    test_two_bit_predictor_counter_updates();
    test_pipeline_trace_csv_output();
    return 0;
}
