#include "decoder.hpp"

namespace rv32i {

namespace {

uint32_t bits(uint32_t value, uint8_t high_bit, uint8_t low_bit) {
    const uint32_t width = high_bit - low_bit + 1;
    const uint32_t mask = (uint32_t{1} << width) - 1;
    return (value >> low_bit) & mask;
}

}  // namespace

uint8_t extract_opcode(uint32_t instruction) {
    return static_cast<uint8_t>(bits(instruction, 6, 0));
}

uint8_t extract_rd(uint32_t instruction) {
    return static_cast<uint8_t>(bits(instruction, 11, 7));
}

uint8_t extract_funct3(uint32_t instruction) {
    return static_cast<uint8_t>(bits(instruction, 14, 12));
}

uint8_t extract_rs1(uint32_t instruction) {
    return static_cast<uint8_t>(bits(instruction, 19, 15));
}

uint8_t extract_rs2(uint32_t instruction) {
    return static_cast<uint8_t>(bits(instruction, 24, 20));
}

uint8_t extract_funct7(uint32_t instruction) {
    return static_cast<uint8_t>(bits(instruction, 31, 25));
}

int32_t sign_extend(uint32_t value, uint8_t bit_count) {
    const uint32_t sign_bit = uint32_t{1} << (bit_count - 1);
    const uint32_t mask = (uint32_t{1} << bit_count) - 1;
    value &= mask;

    if ((value & sign_bit) == 0) {
        return static_cast<int32_t>(value);
    }

    return static_cast<int32_t>(value | ~mask);
}

RTypeFields decode_r_type(uint32_t instruction) {
    return RTypeFields{
        extract_opcode(instruction),
        extract_rd(instruction),
        extract_funct3(instruction),
        extract_rs1(instruction),
        extract_rs2(instruction),
        extract_funct7(instruction),
    };
}

ITypeFields decode_i_type(uint32_t instruction) {
    const uint32_t imm = bits(instruction, 31, 20);

    return ITypeFields{
        extract_opcode(instruction),
        extract_rd(instruction),
        extract_funct3(instruction),
        extract_rs1(instruction),
        sign_extend(imm, 12),
    };
}

STypeFields decode_s_type(uint32_t instruction) {
    const uint32_t imm =
        (bits(instruction, 31, 25) << 5) | bits(instruction, 11, 7);

    return STypeFields{
        extract_opcode(instruction),
        extract_funct3(instruction),
        extract_rs1(instruction),
        extract_rs2(instruction),
        sign_extend(imm, 12),
    };
}

BTypeFields decode_b_type(uint32_t instruction) {
    const uint32_t imm =
        (bits(instruction, 31, 31) << 12) |
        (bits(instruction, 7, 7) << 11) |
        (bits(instruction, 30, 25) << 5) |
        (bits(instruction, 11, 8) << 1);

    return BTypeFields{
        extract_opcode(instruction),
        extract_funct3(instruction),
        extract_rs1(instruction),
        extract_rs2(instruction),
        sign_extend(imm, 13),
    };
}

UTypeFields decode_u_type(uint32_t instruction) {
    return UTypeFields{
        extract_opcode(instruction),
        extract_rd(instruction),
        instruction & 0xFFFFF000u,
    };
}

JTypeFields decode_j_type(uint32_t instruction) {
    const uint32_t imm =
        (bits(instruction, 31, 31) << 20) |
        (bits(instruction, 19, 12) << 12) |
        (bits(instruction, 20, 20) << 11) |
        (bits(instruction, 30, 21) << 1);

    return JTypeFields{
        extract_opcode(instruction),
        extract_rd(instruction),
        sign_extend(imm, 21),
    };
}

}  // namespace rv32i
