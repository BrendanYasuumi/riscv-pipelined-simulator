# Differential byte, halfword, and word memory-access test.

    .section .text
    .globl _start
    .globl golden_halt

_start:
    la   x30, memory_input
    la   x31, memory_results

    lb   x1, 0(x30)
    lbu  x2, 0(x30)
    lh   x3, 2(x30)
    lhu  x4, 2(x30)
    lw   x5, 4(x30)

    sw   x1, 0(x31)
    sw   x2, 4(x31)
    sw   x3, 8(x31)
    sw   x4, 12(x31)
    sw   x5, 16(x31)

    addi x6, x0, -1
    sb   x6, 20(x31)
    sh   x6, 22(x31)

golden_halt:
    ebreak

    .section .data
    .balign 4
memory_input:
    .byte 0x80, 0x7f
    .half 0x8001
    .word 0x89abcdef

    .balign 4
    .globl memory_results
memory_results:
    .space 24
