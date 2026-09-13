.section .text
.globl _start
.globl golden_halt

_start:
    addi x5, x0, 7
    addi x6, x0, 5
    add x7, x5, x6

    la x10, result
    sw x7, 0(x10)


golden_halt:
    ebreak

.section .data
.balign 4
.globl result

result:
    .word 0

