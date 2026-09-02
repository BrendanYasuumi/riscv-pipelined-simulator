# hazard_demo.s
#
# Purpose:
#   Demonstrate a load-use pipeline hazard.
#
# What this program does:
#   1. Load decimal 21 from memory[0x80] into x1.
#   2. Immediately use x1 in the next instruction.
#   3. Store the doubled value, 42, into memory[0x40].
#
# Why this is useful:
#   The instruction right after lw depends on the loaded value. In a pipelined
#   CPU, that value is not ready early enough for the next instruction's EX
#   stage, so the hazard unit freezes fetch/decode for one cycle and inserts a
#   bubble into the pipeline.
#
# Expected final state:
#   memory[0x40] = 2a 00 00 00
#   Stall cycles should include the load-use stall.
#
# Run:
#   make run ASM=asmFiles/hazard_demo.s
#   make trace ASM=asmFiles/hazard_demo.s TRACE_ARGS="--trace --trace-limit=12"

    .section .text
    .globl _start

_start:
    # x10 = address of the input word.
    addi x10, x0, 0x80

    # Load x1 from memory[0x80].
    lw   x1, 0(x10)

    # Load-use hazard:
    # this add needs x1 immediately after the lw.
    add  x2, x1, x1

    # Store the result into memory[0x40].
    addi x11, x0, 0x40
    sw   x2, 0(x11)

    ecall

    .org 0x80
input_word:
    .word 21
