# Differential arithmetic test with positive and negative operands.

    .section .text
    .globl _start
    .globl golden_halt

_start:
    la   x31, arithmetic_results
    addi x1, x0, 40
    addi x2, x0, -7

    add  x3, x1, x2
    sub  x4, x1, x2
    addi x5, x2, -9

    sw   x3, 0(x31)
    sw   x4, 4(x31)
    sw   x5, 8(x31)

golden_halt:
    ebreak

    .section .data
    .balign 4
    .globl arithmetic_results
arithmetic_results:
    .space 12
