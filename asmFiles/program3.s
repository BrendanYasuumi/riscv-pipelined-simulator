# program3.s
#
# Purpose:
#   Estimate days elapsed using:
#       ((currentYear - 2000) * 365) +
#       ((currentMonth - 1) * 30) +
#       currentDay
#
# Inputs:
#   currentYear  = 2025
#   currentMonth = 8
#   currentDay   = 22
#
# Expected with these inputs:
#   year term  = 25 * 365 = 9125
#   month term = 7 * 30   = 210
#   day term   = 22
#   total      = 9357 decimal = 0x248d
#
# Notes:
#   The original assignment used stack address 0xFFFC. This simulator
#   currently has 4096 bytes of RAM, so this version uses stack address 0x300.
#   The linker script already places the program at address 0x0, so this file
#   does not need an "org 0x0000" directive.
#
#   The original note said 9370 / 0x249A, but the arithmetic above evaluates to
#   9357 / 0x248D. This program implements the arithmetic as written.
#
# Run:
#   make run ASM=asmFiles/program3.s

    .section .text
    .globl _start

_start:
    # Initialize stack pointer inside this simulator's 4096-byte RAM.
    addi sp, x0, 0x300

    # t0 = currentYear - 2000 = 25.
    addi t0, x0, 2025
    addi t1, x0, 2000
    sub  t0, t0, t1

    # Push multiplicand = 25 and multiplier = 365.
    addi sp, sp, -8
    sw   t0, 4(sp)
    addi t0, x0, 365
    sw   t0, 0(sp)

    # s2 = 25 * 365 = 9125.
    jal  ra, multiply
    lw   s2, 4(sp)
    addi sp, sp, 8

    # t0 = currentMonth - 1 = 7.
    addi t0, x0, 8
    addi t1, x0, 1
    sub  t0, t0, t1

    # Push multiplicand = 7 and multiplier = 30.
    addi sp, sp, -8
    sw   t0, 4(sp)
    addi t0, x0, 30
    sw   t0, 0(sp)

    # s3 = 7 * 30 = 210.
    jal  ra, multiply
    lw   s3, 4(sp)
    addi sp, sp, 8

    # t0 = currentDay + month term + year term.
    addi t0, x0, 22
    add  t0, t0, s3
    add  t0, t0, s2

    # Store final result to memory so --dump-written-memory shows it.
    addi t4, x0, 0x40
    sw   t0, 0(t4)

    # Stop the simulator.
    ecall

# multiply:
#   Reads two unsigned integers from the stack.
#   Stores the product back at 4(sp), matching the original assignment style.
multiply:
    lw   t0, 0(sp)      # t0 = multiplier
    lw   t1, 4(sp)      # t1 = multiplicand
    addi t2, x0, 0      # t2 = result

multiply_loop:
    beq  t0, x0, end_multiply  # if multiplier == 0, finish
    andi t3, t0, 1             # t3 = multiplier & 1
    beq  t3, x0, skip_add      # if low bit is 0, skip add
    add  t2, t2, t1            # result += multiplicand

skip_add:
    slli t1, t1, 1             # multiplicand <<= 1
    srli t0, t0, 1             # multiplier >>= 1
    jal  x0, multiply_loop     # repeat loop

end_multiply:
    sw   t2, 4(sp)             # write product over multiplicand slot
    jalr x0, 0(ra)             # return to caller
