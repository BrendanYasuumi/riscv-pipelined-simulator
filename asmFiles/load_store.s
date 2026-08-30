    # load_store.s
    #
    # Purpose:
    #   Demonstrate a full register -> memory -> register round trip.
    #
    # What this program does:
    #   1. Put decimal 42 into x1.
    #   2. Put memory address 0x40 into x2.
    #   3. Store x1 into memory[0x40].
    #   4. Load memory[0x40] back into x3.
    #   5. Add 8 to x3 and store the result in x4.
    #   6. Halt with ecall.
    #
    # Expected final state:
    #   x1 = 42
    #   x2 = 0x40
    #   x3 = 42
    #   x4 = 50
    #   memory[0x40] = 2a 00 00 00
    #
    # Run:
    #   make run ASM=asmFiles/load_store.s

    .section .text
    .globl _start

_start:

    ##Add value at register x0 (always 0) to 42 and put in register x1. Register one now holds 0x2a
    addi x1, x0, 42    

    ##Add value at register x0 to 64 and put in register x2. Register x2 now holds 0x40
    addi x2, x0, 64     


    #Store the value at register 1 (0x2a) into the value at register x2 which is a memory address of 0x40 and offset by 0
    #0x40 now contains the value 0x2a
    sw x1, 0(x2)


    #Use the value in x2 as a memory address, then load memory[x2 + 0] into register x3
    #x3 = memory[x2 + 0] --> x3 = memory[0x40] --> x3 = 0x2a
    lw   x3, 0(x2)     

    ##Add the value stored at register x3 (0x2a) with 8 and store it in register x4
    addi x4, x3, 8       

    ecall                
