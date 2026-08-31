    .section .text
    .globl _start

_start:
    addi x1, x0, 0 #counter = 0
    addi x2, x0, 5 #limit = 5
    addi x3, x0, 0x40 #output address

loop:
    addi x1, x1, 1 #counter++


    bne x1, x2, loop #if counter != limit, repeat

    sw x1, 0(x3)
    ecall
