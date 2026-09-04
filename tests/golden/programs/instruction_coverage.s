# Golden-reference ALU and memory coverage.
# Each operation contributes to a memory signature that both models export.

    .section .text
    .globl _start
    .globl golden_halt

_start:
    la   x31, result_signature
    addi x1, x0, 12
    addi x2, x0, 5

    add  x3, x1, x2
    sw   x3, 0(x31)
    sub  x3, x1, x2
    sw   x3, 4(x31)
    and  x3, x1, x2
    sw   x3, 8(x31)
    or   x3, x1, x2
    sw   x3, 12(x31)
    xor  x3, x1, x2
    sw   x3, 16(x31)

    addi x4, x0, 2
    sll  x3, x2, x4
    sw   x3, 20(x31)
    srl  x3, x1, x4
    sw   x3, 24(x31)
    addi x5, x0, -16
    sra  x3, x5, x4
    sw   x3, 28(x31)
    slt  x3, x5, x2
    sw   x3, 32(x31)
    sltu x3, x5, x2
    sw   x3, 36(x31)

    andi  x3, x1, 10
    sw    x3, 40(x31)
    ori   x3, x2, 8
    sw    x3, 44(x31)
    xori  x3, x1, 15
    sw    x3, 48(x31)
    slti  x3, x5, -8
    sw    x3, 52(x31)
    sltiu x3, x2, -1
    sw    x3, 56(x31)
    slli  x3, x2, 3
    sw    x3, 60(x31)
    srli  x3, x5, 2
    sw    x3, 64(x31)
    srai  x3, x5, 2
    sw    x3, 68(x31)

    lui   x3, 0x12345
    sw    x3, 72(x31)
    auipc x3, 0
    sw    x3, 76(x31)

    la   x30, input_data
    lb   x3, 0(x30)
    sw   x3, 80(x31)
    lbu  x3, 0(x30)
    sw   x3, 84(x31)
    lh   x3, 4(x30)
    sw   x3, 88(x31)
    lhu  x3, 4(x30)
    sw   x3, 92(x31)
    lw   x3, 8(x30)
    sw   x3, 96(x31)

    sb   x5, 100(x31)
    sh   x5, 104(x31)
    sw   x3, 108(x31)
    nop

golden_halt:
    # Shared termination point used by the differential-test harness.
    ebreak

    .section .data
    .balign 4
input_data:
    .byte 0x80, 0x7f, 0x00, 0x00
    .half 0x8001, 0x7fff
    .word 0x89abcdef

    .balign 4
    .globl result_signature
result_signature:
    .space 112
