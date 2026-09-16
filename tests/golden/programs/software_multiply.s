# Differential shift-and-add multiplication test: 13 * 9.

    .section .text
    .globl _start
    .globl golden_halt

_start:
    la   x31, multiply_result
    addi x1, x0, 13
    addi x2, x0, 9
    addi x3, x0, 0

multiply_loop:
    beq  x2, x0, multiply_done
    andi x4, x2, 1
    beq  x4, x0, skip_add
    add  x3, x3, x1

skip_add:
    slli x1, x1, 1
    srli x2, x2, 1
    jal  x0, multiply_loop

multiply_done:
    sw   x3, 0(x31)

golden_halt:
    ebreak

    .section .data
    .balign 4
    .globl multiply_result
multiply_result:
    .space 4
