# Differential loop test: sum the integers from 1 through 10.

    .section .text
    .globl _start
    .globl golden_halt

_start:
    la   x31, loop_sum_result
    addi x1, x0, 1
    addi x2, x0, 11
    addi x3, x0, 0

loop:
    add  x3, x3, x1
    addi x1, x1, 1
    bne  x1, x2, loop

    sw   x3, 0(x31)

golden_halt:
    ebreak

    .section .data
    .balign 4
    .globl loop_sum_result
loop_sum_result:
    .space 4
