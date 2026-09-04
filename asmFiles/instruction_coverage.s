# instruction_coverage.s
#
# Purpose:
#   Exercise every supported non-control RV32I instruction and important
#   immediate/load-store edge cases in one deterministic program.
#
# Result signature:
#   The program stores each result beginning at memory address 0x400. The
#   automated assembly test checks those words. EBREAK halts the simulator.
#
# Run:
#   make run ASM=asmFiles/instruction_coverage.s

    .section .text
    .globl _start

_start:
    addi x31, x0, 0x400       # Result signature base.
    addi x1, x0, 12
    addi x2, x0, 5

    add  x3, x1, x2           # 17
    sw   x3, 0(x31)
    sub  x3, x1, x2           # 7
    sw   x3, 4(x31)
    and  x3, x1, x2           # 4
    sw   x3, 8(x31)
    or   x3, x1, x2           # 13
    sw   x3, 12(x31)
    xor  x3, x1, x2           # 9
    sw   x3, 16(x31)

    addi x4, x0, 2
    sll  x3, x2, x4           # 5 << 2 = 20
    sw   x3, 20(x31)
    srl  x3, x1, x4           # 12 >> 2 = 3
    sw   x3, 24(x31)

    addi x5, x0, -16          # Negative 12-bit immediate sign-extension.
    sra  x3, x5, x4           # -16 >> 2 = -4
    sw   x3, 28(x31)
    slt  x3, x5, x2           # Signed: -16 < 5
    sw   x3, 32(x31)
    sltu x3, x5, x2           # Unsigned: 0xfffffff0 < 5 is false.
    sw   x3, 36(x31)

    andi  x3, x1, 10          # 12 & 10 = 8
    sw    x3, 40(x31)
    ori   x3, x2, 8           # 5 | 8 = 13
    sw    x3, 44(x31)
    xori  x3, x1, 15          # 12 ^ 15 = 3
    sw    x3, 48(x31)
    slti  x3, x5, -8          # Signed: -16 < -8
    sw    x3, 52(x31)
    sltiu x3, x2, -1          # Unsigned: 5 < 0xffffffff
    sw    x3, 56(x31)
    slli  x3, x2, 3           # 5 << 3 = 40
    sw    x3, 60(x31)
    srli  x3, x5, 2           # Logical shift inserts zeroes.
    sw    x3, 64(x31)
    srai  x3, x5, 2           # Arithmetic shift preserves the sign.
    sw    x3, 68(x31)

    lui  x3, 0x12345          # x3 = 0x12345000
    sw   x3, 72(x31)

    # Jump to a fixed address so AUIPC has a simple expected result.
    jal  x0, upper_immediate_test

    .org 0x200
upper_immediate_test:
    auipc x3, 0               # x3 = this instruction's PC = 0x200
    sw    x3, 76(x31)

    addi x30, x0, 0x300       # Input data base.
    lb   x3, 0(x30)           # Sign-extend 0x80.
    sw   x3, 80(x31)
    lbu  x3, 0(x30)           # Zero-extend 0x80.
    sw   x3, 84(x31)
    lh   x3, 4(x30)           # Sign-extend 0x8001.
    sw   x3, 88(x31)
    lhu  x3, 4(x30)           # Zero-extend 0x8001.
    sw   x3, 92(x31)
    lw   x3, 8(x30)           # Load the complete 32-bit word.
    sw   x3, 96(x31)

    sb   x5, 100(x31)         # Store low byte: 0xf0.
    sh   x5, 104(x31)         # Store low halfword: 0xfff0.
    sw   x3, 108(x31)         # Store complete word: 0x89abcdef.

    nop                       # Encoded as addi x0, x0, 0.
    ebreak                    # Supported simulator halt instruction.

    .org 0x300
input_data:
    .byte 0x80, 0x7f, 0x00, 0x00
    .half 0x8001, 0x7fff
    .word 0x89abcdef
