# Golden-reference control-flow coverage.
# Both taken and not-taken paths are exercised for every branch condition.

    .section .text
    .globl _start
    .globl golden_halt

_start:
    la   x31, control_results
    addi x1, x0, 0
    addi x2, x0, 5
    addi x3, x0, 5
    addi x4, x0, 7
    addi x5, x0, -1
    addi x6, x0, 1

    beq  x2, x4, fail
    beq  x2, x3, beq_pass
    jal  x0, fail
beq_pass:
    addi x1, x1, 1

    bne  x2, x3, fail
    bne  x2, x4, bne_pass
    jal  x0, fail
bne_pass:
    addi x1, x1, 2

    blt  x6, x5, fail
    blt  x5, x6, blt_pass
    jal  x0, fail
blt_pass:
    addi x1, x1, 4

    bge  x5, x6, fail
    bge  x6, x5, bge_pass
    jal  x0, fail
bge_pass:
    addi x1, x1, 8

    bltu x5, x6, fail
    bltu x6, x5, bltu_pass
    jal  x0, fail
bltu_pass:
    addi x1, x1, 16

    bgeu x6, x5, fail
    bgeu x5, x6, bgeu_pass
    jal  x0, fail
bgeu_pass:
    addi x1, x1, 32

    jal  x7, jal_pass
    jal  x0, fail
jal_pass:
    addi x1, x1, 64
    sw   x7, 4(x31)

    la   x8, jalr_target
    jalr x9, 0(x8)
    jal  x0, fail

fail:
    addi x1, x0, -1
    sw   x1, 0(x31)
    jal  x0, golden_halt

jalr_target:
    addi x1, x1, 128
    sw   x1, 0(x31)
    sw   x9, 8(x31)

golden_halt:
    # Shared termination point used by the differential-test harness.
    ebreak

    .section .data
    .balign 4
    .globl control_results
control_results:
    .space 12
