# Differential logical and arithmetic shift test.

    .section .text
    .globl _start
    .globl golden_halt

_start:
    la   x31, shift_results
    addi x1, x0, 1
    addi x2, x0, 4
    lui  x3, 0x80000

    sll  x4, x1, x2
    slli x5, x1, 7
    srl  x6, x3, x2
    sra  x7, x3, x2
    srli x8, x3, 8
    srai x9, x3, 8

    sw   x4, 0(x31)
    sw   x5, 4(x31)
    sw   x6, 8(x31)
    sw   x7, 12(x31)
    sw   x8, 16(x31)
    sw   x9, 20(x31)

golden_halt:
    ebreak

    .section .data
    .balign 4
    .globl shift_results
shift_results:
    .space 24
