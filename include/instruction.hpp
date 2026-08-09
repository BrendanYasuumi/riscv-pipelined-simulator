#pragma once

#include <cstdint>

#include "pipeline_registers.hpp"

namespace rv32i {

enum class InstructionKind {
    Invalid,
    Add,
    Sub,
    Sll,
    Slt,
    Sltu,
    Xor,
    Srl,
    Sra,
    Or,
    And,
    Addi,
    Slti,
    Sltiu,
    Xori,
    Ori,
    Andi,
    Slli,
    Srli,
    Srai,
    Lb,
    Lh,
    Lw,
    Lbu,
    Lhu,
    Sb,
    Sh,
    Sw,
    Beq,
    Bne,
    Blt,
    Bge,
    Bltu,
    Bgeu,
    Lui,
    Auipc,
    Jal,
    Jalr,
    Ecall,
    Ebreak
};

struct DecodedInstruction {
    InstructionKind kind = InstructionKind::Invalid;
    InstructionFormat format = InstructionFormat::Unknown;
    uint32_t raw = 0;

    uint8_t opcode = 0;
    uint8_t rd = 0;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
    uint8_t funct3 = 0;
    uint8_t funct7 = 0;
    int32_t immediate = 0;

    ControlSignals control{};
};

DecodedInstruction decode_instruction(uint32_t instruction);
const char* instruction_kind_name(InstructionKind kind);
bool is_valid_instruction(const DecodedInstruction& instruction);

}  // namespace rv32i
