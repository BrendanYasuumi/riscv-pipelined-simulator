.section .text
.globl _start

_start:
    addi x1, x0, 5       # x1 = 5
    addi x2, x0, 7       # x2 = 7

    addi x3, x0, 0x40    # x3 = output memory address

    beq x1, x2, equal    # if x1 == x2, jump to equal

    # Not equal path
    addi x4, x0, 0       # result = 0
    jal  x0, done        # skip equal path

equal:
    # Equal path
    addi x4, x0, 1       # result = 1

done:
    sw x4, 0(x3)         # store result to memory[0x40]
    ecall                # halt