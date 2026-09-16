# Differential register and immediate bitwise test.

    .section .text
    .globl _start
    .globl golden_halt

_start:
    la   x31, bitwise_results
    addi x1, x0, 0x5a5
    addi x2, x0, 0x3c3

    and  x3, x1, x2
    or   x4, x1, x2
    xor  x5, x1, x2
    andi x6, x1, 0x0f
    ori  x7, x2, 0x40
    xori x8, x1, -1

    sw   x3, 0(x31)
    sw   x4, 4(x31)
    sw   x5, 8(x31)
    sw   x6, 12(x31)
    sw   x7, 16(x31)
    sw   x8, 20(x31)

golden_halt:
    ebreak

    .section .data
    .balign 4
    .globl bitwise_results
bitwise_results:
    .space 24
