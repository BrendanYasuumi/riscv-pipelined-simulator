# Differential forwarding and load-use hazard test.

    .section .text
    .globl _start
    .globl golden_halt

_start:
    la   x30, hazard_input
    la   x31, hazard_results

    lw   x1, 0(x30)
    add  x2, x1, x1
    add  x3, x2, x1
    addi x4, x3, -1

    sw   x2, 0(x31)
    sw   x3, 4(x31)
    sw   x4, 8(x31)

golden_halt:
    ebreak

    .section .data
    .balign 4
hazard_input:
    .word 21

    .globl hazard_results
hazard_results:
    .space 12
