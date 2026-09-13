    .section .text
    .globl _start

_start:
    # Exercise positive and negative immediates.
    addi x5, x0, 10
    addi x6, x0, -3

    # Expected results: 10 + (-3) = 7 and 10 - (-3) = 13.
    add  x7, x5, x6
    sub  x8, x5, x6

    # Store both results where the test runner can inspect them.
    addi x10, x0, 0x40
    sw   x7, 0(x10)
    sw   x8, 4(x10)

    ebreak

    .org 0x40
results:
    .word 0
    .word 0
