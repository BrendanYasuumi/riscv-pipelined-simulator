    .section .text
    .globl _start

_start:
    addi x1, x0, 12
    addi x2, x0, 10

    and x3, x1, x2
    or x4, x1, x2
    xor x5, x1, x2

    addi x10, x0, 0x40 #x10 = output base address

    sw x3, 0(x10) #memory[0x40] = 8
    sw x4, 4(x10) #memory [0x44] = 14
    sw x5, 8(x10) #memory[0x48] = 6

    ecall
