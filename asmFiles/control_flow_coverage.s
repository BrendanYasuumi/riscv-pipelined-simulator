# control_flow_coverage.s
#
# Purpose:
#   Exercise taken and not-taken behavior for all six RV32I branches, then
#   verify JAL and JALR control transfer and link-register values.
#
# Expected final signature:
#   memory[0x500] = 255 (all branch/jump checks passed)
#   memory[0x504] = JAL return address
#   memory[0x508] = JALR return address
#
# Run:
#   make run ASM=asmFiles/control_flow_coverage.s

    .section .text
    .globl _start

_start:
    addi x31, x0, 0x500       # Result signature base.
    addi x1, x0, 0            # Success signature accumulator.
    addi x2, x0, 5
    addi x3, x0, 5
    addi x4, x0, 7
    addi x5, x0, -1
    addi x6, x0, 1

    beq  x2, x4, fail         # Not taken: 5 != 7.
    beq  x2, x3, beq_pass     # Taken: 5 == 5.
    jal  x0, fail
beq_pass:
    addi x1, x1, 1

    bne  x2, x3, fail         # Not taken: 5 == 5.
    bne  x2, x4, bne_pass     # Taken: 5 != 7.
    jal  x0, fail
bne_pass:
    addi x1, x1, 2

    blt  x6, x5, fail         # Signed, not taken: 1 is not less than -1.
    blt  x5, x6, blt_pass     # Signed, taken: -1 < 1.
    jal  x0, fail
blt_pass:
    addi x1, x1, 4

    bge  x5, x6, fail         # Signed, not taken: -1 is not >= 1.
    bge  x6, x5, bge_pass     # Signed, taken: 1 >= -1.
    jal  x0, fail
bge_pass:
    addi x1, x1, 8

    bltu x5, x6, fail         # Unsigned, not taken: 0xffffffff is not < 1.
    bltu x6, x5, bltu_pass    # Unsigned, taken: 1 < 0xffffffff.
    jal  x0, fail
bltu_pass:
    addi x1, x1, 16

    bgeu x6, x5, fail         # Unsigned, not taken: 1 is not >= 0xffffffff.
    bgeu x5, x6, bgeu_pass    # Unsigned, taken: 0xffffffff >= 1.
    jal  x0, fail
bgeu_pass:
    addi x1, x1, 32

    jal  x7, jal_pass         # x7 receives the address after this JAL.
    jal  x0, fail             # Must be flushed.
jal_pass:
    addi x1, x1, 64
    sw   x7, 4(x31)

    addi x8, x0, 0x200        # Fixed JALR target.
    jalr x9, 0(x8)            # x9 receives the address after this JALR.
    jal  x0, fail             # Must be flushed.

fail:
    addi x1, x0, -1
    sw   x1, 0(x31)
    ecall

    .org 0x200
jalr_target:
    addi x1, x1, 128
    sw   x1, 0(x31)
    sw   x9, 8(x31)
    ecall
