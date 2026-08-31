# program1.s
#
# Purpose:
#   Multiply two unsigned numbers without using a hardware multiply
#   instruction. The algorithm uses shifts and adds.
#
# Inputs:
#   multiplicand = 6
#   multiplier   = 7
#
# Expected:
#   a0 = 42
#   memory[0x40] = 2a 00 00 00
#
# Notes:
#   The original assignment used stack address 0xFFFC. This simulator
#   currently has 4096 bytes of RAM, so this version uses stack address 0x300.
#   The linker script already places the program at address 0x0, so this file
#   does not need an "org 0x0000" directive.
#
# Run:
#   make run ASM=asmFiles/program1.s

    .section .text
    .globl _start

_start:
    # Initialize stack pointer to 0x300.
    addi sp, x0, 0x300

    # Reserve 8 bytes for two 32-bit arguments.
    addi sp, sp, -8

    # Push first number: multiplicand = 6.
    addi t0, x0, 6
    sw   t0, 4(sp)

    # Push second number: multiplier = 7.
    addi t0, x0, 7
    sw   t0, 0(sp)

    # Call multiply. jal stores the return address in ra.
    jal  ra, multiply

    # Clean up stack after the function returns.
    addi sp, sp, 8

    # Store the return value a0 to memory so the simulator can show it.
    addi t4, x0, 0x40
    sw   a0, 0(t4)

    # Stop the simulator.
    ecall

# multiply:
#   Reads two unsigned integers from the stack and returns product in a0.
multiply:
    # Load arguments from stack.
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
    addi a0, t2, 0             # return value = result
    jalr x0, 0(ra)             # return to caller
