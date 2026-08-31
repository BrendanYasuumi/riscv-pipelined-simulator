# search.s
#
# Purpose:
#   Search a list of 100 words for the value 0x5c6f.
#
# Expected:
#   The value 0x5c6f is found at memory address 0x124.
#   The program stores that address into memory[0x80].
#
# Expected final written memory:
#   memory[0x80] = 24 01 00 00
#
# Run:
#   make run ASM=asmFiles/search.s

    .section .text
    .globl _start

_start:
    # x2 points to the beginning of the data block at 0x80.
    ori   x2, x0, 0x80

start:
    ori   x4, x0, 0x01      # x4 = 1, used to decrement list length
    ori   x10, x0, 0x04     # x10 = 4, used to move to next word

    sw    x0, 0(x2)         # result = 0 until item is found
    lw    x11, 4(x2)        # x11 = search item
    lw    x12, 8(x2)        # x12 = list length
    addi  x13, x2, 12       # x13 = pointer to first list element

loop:
    lw    x14, 0(x13)       # x14 = current list element
    sub   x15, x14, x11     # x15 = current element - search item
    beq   x15, x0, found    # if x15 == 0, item matched

    add   x13, x13, x10     # move pointer to next list element
    sub   x12, x12, x4      # remaining length -= 1
    beq   x12, x0, notfound # if length == 0, item was not found
    beq   x0, x0, loop      # unconditional branch back to loop

found:
    sw    x13, 0(x2)        # store found address into memory[0x80]

notfound:
    ecall                   # halt simulator

    .org 0x80
item_position:
    .word 0                 # result slot; expected final value is 0x124
search_item:
    .word 0x5c6f
list_length:
    .word 100
search_list:
    .word 0x087d
    .word 0x5fcb
    .word 0xa41a
    .word 0x4109
    .word 0x4522
    .word 0x700f
    .word 0x766d
    .word 0x6f60
    .word 0x8a5e
    .word 0x9580
    .word 0x70a3
    .word 0xaea9
    .word 0x711a
    .word 0x6f81
    .word 0x8f9a
    .word 0x2584
    .word 0xa599
    .word 0x4015
    .word 0xce81
    .word 0xf55b
    .word 0x399e
    .word 0xa23f
    .word 0x3588
    .word 0x33ac
    .word 0xbce7
    .word 0x2a6b
    .word 0x9fa1
    .word 0xc94b
    .word 0xc65b
    .word 0x0068
    .word 0xf499
    .word 0x5f71
    .word 0xd06f
    .word 0x14df
    .word 0x1165
    .word 0xf88d
    .word 0x4ba4
    .word 0x2e74
    .word 0x5c6f
    .word 0xd11e
    .word 0x9222
    .word 0xacdb
    .word 0x1038
    .word 0xab17
    .word 0xf7ce
    .word 0x8a9e
    .word 0x9aa3
    .word 0xb495
    .word 0x8a5e
    .word 0xd859
    .word 0x0bac
    .word 0xd0db
    .word 0x3552
    .word 0xa6b0
    .word 0x727f
    .word 0x28e4
    .word 0xe5cf
    .word 0x163c
    .word 0x3411
    .word 0x8f07
    .word 0xfab7
    .word 0x0f34
    .word 0xdabf
    .word 0x6f6f
    .word 0xc598
    .word 0xf496
    .word 0x9a9a
    .word 0xbd6a
    .word 0x2136
    .word 0x810a
    .word 0xca55
    .word 0x8bce
    .word 0x2ac4
    .word 0xddce
    .word 0xdd06
    .word 0xc4fc
    .word 0xfb2f
    .word 0xee5f
    .word 0xfd30
    .word 0xc540
    .word 0xd5f1
    .word 0xbdad
    .word 0x45c3
    .word 0x708a
    .word 0xa359
    .word 0xf40d
    .word 0xba06
    .word 0xbace
    .word 0xb447
    .word 0x3f48
    .word 0x899e
    .word 0x8084
    .word 0xbdb9
    .word 0xa05a
    .word 0xe225
    .word 0xfb0c
    .word 0xb2b2
    .word 0xa4db
    .word 0x8bf9
    .word 0x12f7
