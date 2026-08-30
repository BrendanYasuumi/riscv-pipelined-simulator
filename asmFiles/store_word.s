    .section .text
    .globl _start

_start:
    # x4 = 24
    addi x4, x0, 24

    # x10 = 16, which points to address 0x10.
    addi x10, x0, 16

    # Store x4 into memory[0x10].
    sw x4, 0(x10)

    # Stop the simulator.
    ecall

    # Place one data word at byte address 0x10.
    .org 0x10
data_word:
    .word 0
