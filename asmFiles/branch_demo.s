# branch_demo.s
#
# Purpose:
#   Demonstrate a taken branch control hazard.
#
# What this program does:
#   1. Set x1 = 5 and x2 = 5.
#   2. Execute beq x1, x2, taken.
#   3. Because the branch is taken, skip the wrong-path ecall.
#   4. Store 1 into memory[0x40].
#
# Why this is useful:
#   The fetch stage keeps reading PC + 4 while the branch is still moving
#   through the pipeline. When the branch resolves in EX, the simulator must
#   redirect the PC to the branch target and flush the wrong-path instruction.
#
# Expected final state:
#   memory[0x40] = 01 00 00 00
#
# Run:
#   make run ASM=asmFiles/branch_demo.s
#   make trace ASM=asmFiles/branch_demo.s TRACE_ARGS="--trace --trace-limit=12"

    .section .text
    .globl _start

_start:
    # x1 and x2 are equal, so the beq below should be taken.
    addi x1, x0, 5
    addi x2, x0, 5

    # x3 = output memory address.
    addi x3, x0, 0x40

    # Branch should redirect execution to taken.
    beq  x1, x2, taken

    # Wrong path:
    # if branch flushing is broken, this halt may retire before the store.
    ecall

taken:
    addi x4, x0, 1
    sw   x4, 0(x3)
    ecall
