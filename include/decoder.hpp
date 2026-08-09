#pragma once

#include <cstdint>

namespace rv32i {


//Different Instruction Set Types
struct RTypeFields {
    uint8_t opcode = 0;
    uint8_t rd = 0;
    uint8_t funct3 = 0;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
    uint8_t funct7 = 0;
};

struct ITypeFields {
    uint8_t opcode = 0;
    uint8_t rd = 0;
    uint8_t funct3 = 0;
    uint8_t rs1 = 0;
    int32_t imm = 0;
};

struct STypeFields {
    uint8_t opcode = 0;
    uint8_t funct3 = 0;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
    int32_t imm = 0;
};

struct BTypeFields {
    uint8_t opcode = 0;
    uint8_t funct3 = 0;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
    int32_t imm = 0;
};

struct UTypeFields {
    uint8_t opcode = 0;
    uint8_t rd = 0;
    uint32_t imm = 0;
};

struct JTypeFields {
    uint8_t opcode = 0;
    uint8_t rd = 0;
    int32_t imm = 0;
};

uint8_t extract_opcode(uint32_t instruction);
uint8_t extract_rd(uint32_t instruction);
uint8_t extract_funct3(uint32_t instruction);
uint8_t extract_rs1(uint32_t instruction);
uint8_t extract_rs2(uint32_t instruction);
uint8_t extract_funct7(uint32_t instruction);

int32_t sign_extend(uint32_t value, uint8_t bit_count);

RTypeFields decode_r_type(uint32_t instruction);
ITypeFields decode_i_type(uint32_t instruction);
STypeFields decode_s_type(uint32_t instruction);
BTypeFields decode_b_type(uint32_t instruction);
UTypeFields decode_u_type(uint32_t instruction);
JTypeFields decode_j_type(uint32_t instruction);

}  // namespace rv32i
