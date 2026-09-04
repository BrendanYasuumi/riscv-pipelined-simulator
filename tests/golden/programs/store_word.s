# Golden-reference smoke test.
# The labels make this program relocatable so the same linked image can run on
# both Spike and the pipelined simulator.

    .section .text
    .globl _start
    .globl golden_halt

_start:
    addi x4, x0, 24
    la   x10, result
    sw   x4, 0(x10)

golden_halt:
    # The harness stops Spike here. Our simulator retires EBREAK as halt.
    ebreak

    .section .data
    .balign 4
    .globl result
result:
    .word 0
