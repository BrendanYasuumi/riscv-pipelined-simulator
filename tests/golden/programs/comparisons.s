# Differential signed and unsigned comparison test.

    .section .text
    .globl _start
    .globl golden_halt

_start:
    la   x31, comparison_results
    addi x1, x0, -1
    addi x2, x0, 1

    slt   x3, x1, x2
    sltu  x4, x1, x2
    slti  x5, x2, 2
    sltiu x6, x1, 1

    sw    x3, 0(x31)
    sw    x4, 4(x31)
    sw    x5, 8(x31)
    sw    x6, 12(x31)

golden_halt:
    ebreak

    .section .data
    .balign 4
    .globl comparison_results
comparison_results:
    .space 16
