# fibonacci.s
#
# Purpose:
#   Generate a Fibonacci sequence in memory.
#
# Program behavior:
#   memory[0x80] = fib[0] = 0
#   memory[0x84] = fib[1] = 1
#   The loop computes the next 22 Fibonacci values and stores them after that.
#
# Expected final written memory:
#   The simulator should write fib[2] through fib[23] starting at memory[0x88].
#   The last generated value is fib[23] = 28657 decimal = 0x6ff1.
#
# Notes:
#   This file uses GNU RISC-V assembler syntax:
#     .org  replaces org
#     .word replaces cfw
#     ecall replaces halt in this simulator
#     x10   replaces $10-style register spelling
#
# Run:
#   make run ASM=asmFiles/fibonacci.s

    .section .text
    .globl _start

_start:
    # x10 points to the first Fibonacci word at memory address 0x80.
    addi x10, x0, 0x80

    # x4 = 1, used to decrement the loop counter.
    addi x4, x0, 1

    # x5 = 4, the number of bytes in one 32-bit word.
    addi x5, x0, 4

    # Build address 0x0f00 in x14.
    # RV32I immediates are 12-bit signed values, so 0x0f00 cannot be placed
    # directly in an addi/ori instruction. Instead:
    #   lui  x14, 0x1      -> x14 = 0x1000
    #   addi x14, x14, -256 -> x14 = 0x0f00
    lui  x14, 0x1
    addi x14, x14, -256

    # x16 = loop count loaded from memory[0x0f00].
    lw   x16, 0(x14)

loop:
    # Load two adjacent Fibonacci values.
    lw   x11, 0(x10)
    lw   x12, 4(x10)

    # Compute the next value and store it one word after the pair.
    add  x13, x11, x12
    sw   x13, 8(x10)

    # Advance the pointer by one word and decrement the loop counter.
    add  x10, x10, x5
    sub  x16, x16, x4

    # Continue until x16 reaches zero.
    bne  x16, x0, loop

end:
    ecall

    .org 0x80
start:
    .word 0
    .word 1

    .org 0x0f00
fib_count:
    .word 22
