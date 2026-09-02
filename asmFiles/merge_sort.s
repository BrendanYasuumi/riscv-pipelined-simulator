# merge_sort.s
#
# Purpose:
#   Sort 64 integer words using the original benchmark structure:
#     1. insertion-sort the left half
#     2. insertion-sort the right half
#     3. merge both sorted halves into a separate output buffer
#
# Data layout:
#   memory[0x300] = size = 64
#   memory[0x304] = first input element
#   memory[0x500] = first sorted output element
#
# Expected:
#   The sorted output buffer at memory[0x500] should contain 64 words in
#   ascending order.
#
# Notes:
#   This is a GNU RISC-V assembler version of the classroom-style source:
#     org   -> .org
#     cfw   -> .word
#     push  -> addi sp, sp, -4 / sw register, 0(sp)
#     j     -> jal x0, label
#     jr ra -> jalr x0, 0(ra)
#
#   The original source built a high stack address using several instructions.
#   This simulator has 64 KiB of RAM, so this version places the stack at
#   0x700, safely above the input and output arrays.
#
# Run:
#   make run ASM=asmFiles/merge_sort.s SIM_ARGS=--max-cycles=100000

    .section .text
    .globl _start

_start:
    # Stack pointer: keep it above the arrays at 0x300 and 0x500.
    addi sp, x0, 0x700

    # x12 = pointer to input data.
    addi x12, x0, 0x304

    # x24 = size.
    addi x25, x0, 0x300
    lw   x24, 0(x25)

    # x13 = size / 2.
    addi x6, x0, 1
    srl  x13, x24, x6

    # Save left-half arguments for the later merge call.
    or   x9, x0, x12
    or   x18, x0, x13

    # Sort first half:
    #   x12 = pointer to first half
    #   x13 = number of elements in first half
    jal  ra, insertion_sort

    # Compute second half size and pointer.
    addi x6, x0, 1
    srl  x5, x24, x6
    sub  x13, x24, x5

    addi x6, x0, 2
    sll  x5, x5, x6
    addi x12, x0, 0x304
    add  x12, x12, x5

    # Save right-half arguments for the later merge call.
    or   x19, x0, x12
    or   x20, x0, x13

    # Sort second half:
    #   x12 = pointer to second half
    #   x13 = number of elements in second half
    jal  ra, insertion_sort

    # Prepare merge arguments:
    #   x12 = left pointer
    #   x13 = left count
    #   x14 = right pointer
    #   x15 = right count
    or   x12, x0, x9
    or   x13, x0, x18
    or   x14, x0, x19
    or   x15, x0, x20

    # Push destination pointer for merge.
    addi x5, x0, 0x500
    addi sp, sp, -4
    sw   x5, 0(sp)

    jal  ra, merge

    # Pop destination pointer argument.
    addi sp, sp, 4

    ecall

# insertion_sort:
#   x12 = base pointer
#   x13 = element count
#
# Local register meanings:
#   x5  = byte offset of current outer element
#   x6  = total byte size of the array
#   x30 = value being inserted
#   x31 = scan pointer
insertion_sort:
    addi x5, x0, 4
    addi x7, x0, 2
    sll  x6, x13, x7

is_outer:
    sltu x4, x5, x6
    beq  x4, x0, is_end

    add  x31, x12, x5
    lw   x30, 0(x31)

is_inner:
    beq  x31, x12, is_inner_end

    lw   x16, -4(x31)
    slt  x4, x30, x16
    beq  x4, x0, is_inner_end

    sw   x16, 0(x31)
    addi x31, x31, -4
    jal  x0, is_inner

is_inner_end:
    sw   x30, 0(x31)
    addi x5, x5, 4
    jal  x0, is_outer

is_end:
    jalr x0, 0(ra)

# merge:
#   x12 = left pointer
#   x13 = left count
#   x14 = right pointer
#   x15 = right count
#   0(sp) = destination pointer
merge:
    lw   x5, 0(sp)

m_1:
    bne  x13, x0, m_3

m_2:
    bne  x15, x0, m_3
    jal  x0, m_end

m_3:
    beq  x15, x0, m_4
    beq  x13, x0, m_5

    lw   x6, 0(x12)
    lw   x7, 0(x14)
    slt  x4, x6, x7
    beq  x4, x0, m_3a

    sw   x6, 0(x5)
    addi x5, x5, 4
    addi x12, x12, 4
    addi x13, x13, -1
    jal  x0, m_1

m_3a:
    sw   x7, 0(x5)
    addi x5, x5, 4
    addi x14, x14, 4
    addi x15, x15, -1
    jal  x0, m_1

m_4:
    # Left copy: right half is empty, copy the rest of the left half.
    lw   x6, 0(x12)
    sw   x6, 0(x5)
    addi x5, x5, 4
    addi x13, x13, -1
    addi x12, x12, 4
    beq  x13, x0, m_end
    jal  x0, m_4

m_5:
    # Right copy: left half is empty, copy the rest of the right half.
    lw   x7, 0(x14)
    sw   x7, 0(x5)
    addi x5, x5, 4
    addi x15, x15, -1
    addi x14, x14, 4
    beq  x15, x0, m_end
    jal  x0, m_5

m_end:
    jalr x0, 0(ra)

    .org 0x300
size:
    .word 64
data:
    .word 90
    .word 81
    .word 51
    .word 25
    .word 80
    .word 41
    .word 22
    .word 21
    .word 12
    .word 62
    .word 75
    .word 71
    .word 83
    .word 81
    .word 77
    .word 22
    .word 11
    .word 29
    .word 7
    .word 33
    .word 99
    .word 27
    .word 100
    .word 66
    .word 61
    .word 32
    .word 1
    .word 54
    .word 4
    .word 61
    .word 56
    .word 3
    .word 48
    .word 8
    .word 66
    .word 100
    .word 15
    .word 92
    .word 65
    .word 32
    .word 9
    .word 47
    .word 89
    .word 17
    .word 7
    .word 35
    .word 68
    .word 32
    .word 10
    .word 7
    .word 23
    .word 92
    .word 91
    .word 40
    .word 26
    .word 8
    .word 36
    .word 38
    .word 8
    .word 38
    .word 16
    .word 50
    .word 7
    .word 67

    .org 0x500
sorted:
    .space 256
