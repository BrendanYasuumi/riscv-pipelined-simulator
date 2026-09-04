# forwarding_demo.s
#
# Purpose:
#   Demonstrate ALU-to-ALU data forwarding without a pipeline stall.
#
# What this program does:
#   1. Put decimal 42 in x1.
#   2. Immediately add x1 to itself and place decimal 84 in x2.
#   3. Store x2 into memory[0x40].
#
# Why this is useful:
#   The add instruction reaches EX before addi has written x1 back to the
#   register file. The forwarding unit sends the newer addi result directly
#   from EX/MEM to both ALU inputs used by add, avoiding a stall.
#
# Expected final state:
#   memory[0x40] = 54 00 00 00 (84 decimal, little-endian)
#   Stall cycles = 0
#
# Run:
#   make run ASM=asmFiles/forwarding_demo.s
#   make trace ASM=asmFiles/forwarding_demo.s TRACE_ARGS="--trace --trace-limit=10"

    .section .text
    .globl _start

_start:
    # Address where the result will be stored.
    addi x10, x0, 0x40

    # Produce 42 in the EX stage.
    addi x1, x0, 42

    # Immediately consume x1. Both operands are forwarded from EX/MEM.
    add  x2, x1, x1

    # Store 84 at memory address 0x40.
    sw   x2, 0(x10)

    ecall
