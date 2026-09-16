# Differential loop and memory test: write the first ten Fibonacci values.

    .section .text
    .globl _start
    .globl golden_halt

_start:
    la   x31, fibonacci_results
    addi x1, x0, 0
    addi x2, x0, 1
    addi x3, x0, 10
    addi x4, x31, 0

fibonacci_loop:
    sw   x1, 0(x4)
    add  x5, x1, x2
    addi x1, x2, 0
    addi x2, x5, 0
    addi x4, x4, 4
    addi x3, x3, -1
    bne  x3, x0, fibonacci_loop

golden_halt:
    ebreak

    .section .data
    .balign 4
    .globl fibonacci_results
fibonacci_results:
    .space 40
