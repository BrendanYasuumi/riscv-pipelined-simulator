# Differential JAL/JALR function-call and return test.

    .section .text
    .globl _start
    .globl golden_halt

_start:
    la   x31, function_results
    addi x10, x0, 19
    addi x11, x0, 23
    jal  x1, add_pair

    sw   x10, 0(x31)
    sw   x1, 4(x31)
    jal  x0, golden_halt

add_pair:
    add  x10, x10, x11
    jalr x0, 0(x1)

golden_halt:
    ebreak

    .section .data
    .balign 4
    .globl function_results
function_results:
    .space 8
