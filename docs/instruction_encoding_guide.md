# RV32I Instruction Encoding Guide

This guide explains how small RV32I assembly instructions become the 32-bit
machine-code words used in `.hex` example programs.

## What `.hex` Means Here

A `.hex` file is a text file containing already-encoded machine instructions.
Each non-comment line is one 32-bit instruction word:

```text
00500093
00700113
002081b3
```

The simulator loads each word into byte-addressable memory in little-endian
order:

```text
hex word:        00500093
instruction:     0x00500093
memory bytes:    93 00 50 00
```

## Register Numbers

```text
x0  = 0
x1  = 1
x2  = 2
x3  = 3
...
x31 = 31
```

`x0` is hardwired to zero. Reads always return zero, and writes are ignored.

## R-Type: Register-Register ALU

Example:

```text
add x3, x1, x2
```

Bit layout:

```text
31..25  24..20  19..15  14..12  11..7   6..0
funct7  rs2     rs1     funct3  rd      opcode
```

Fields:

```text
funct7 = 0x00
rs2    = 2
rs1    = 1
funct3 = 0x0
rd     = 3
opcode = 0x33
```

Encoding formula:

```text
(funct7 << 25) | (rs2 << 20) | (rs1 << 15) |
(funct3 << 12) | (rd << 7) | opcode
```

Result:

```text
0x002081b3
```

## I-Type: Immediate ALU and Loads

Example:

```text
addi x1, x0, 5
```

Bit layout:

```text
31..20  19..15  14..12  11..7   6..0
imm     rs1     funct3  rd      opcode
```

Fields:

```text
imm    = 5
rs1    = 0
funct3 = 0x0
rd     = 1
opcode = 0x13
```

Result:

```text
0x00500093
```

Loads use the same general format with opcode `0x03`.

## S-Type: Stores

Example:

```text
sw x2, 0(x1)
```

Bit layout:

```text
31..25    24..20  19..15  14..12  11..7     6..0
imm[11:5] rs2     rs1     funct3  imm[4:0]  opcode
```

Stores split the immediate because there is no destination register. The stored
value comes from `rs2`, and the base address comes from `rs1`.

## B-Type: Branches

Example:

```text
beq x0, x0, +8
```

Bit layout:

```text
31      30..25    24..20  19..15  14..12  11..8     7       6..0
imm[12] imm[10:5] rs2     rs1     funct3  imm[4:1]  imm[11] opcode
```

Branch offsets are PC-relative. The lowest offset bit is not stored because
branch targets are aligned.

## U-Type: LUI and AUIPC

Example:

```text
lui x1, 0x12345
```

Bit layout:

```text
31..12  11..7  6..0
imm     rd     opcode
```

Result:

```text
0x123450b7
```

## J-Type: JAL

Example:

```text
jal x1, +8
```

Bit layout:

```text
31      30..21     20       19..12      11..7  6..0
imm[20] imm[10:1]  imm[11]  imm[19:12]  rd     opcode
```

Like branches, jump offsets are PC-relative and omit the lowest offset bit.

## Practical Examples

```text
addi x1, x0, 5   -> 00500093
addi x2, x0, 7   -> 00700113
add  x3, x1, x2  -> 002081b3
```

These three words form `examples/add.hex`.
