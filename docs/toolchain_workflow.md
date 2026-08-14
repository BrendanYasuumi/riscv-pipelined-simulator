# RISC-V Toolchain Workflow Notes

The simulator can load either simple `.hex` files or raw `.bin` files. These
formats are useful because they let you separate two jobs:

- An assembler or compiler turns human-readable code into RV32I machine code.
- This simulator models how that machine code moves through a pipeline.

## What `.hex` Means Here

A `.hex` file in this project is a text file with one 32-bit instruction word
per line:

```text
00500093
00700113
002081b3
```

Those values are already machine code, not assembly. The simulator converts
each word into four little-endian bytes in simulated RAM.

## What `.bin` Means Here

A `.bin` file is raw bytes exactly as they should appear in memory. If the
first instruction is `0x00500093`, the first four bytes in the file are:

```text
93 00 50 00
```

## Possible GNU Toolchain Flow

Toolchain package names differ by system, but the command names often look like
`riscv64-unknown-elf-*`.

Example assembly file:

```asm
    .text
    addi x1, x0, 5
    addi x2, x0, 7
    add  x3, x1, x2
```

Assemble for RV32I:

```bash
riscv64-unknown-elf-as -march=rv32i -mabi=ilp32 program.s -o program.o
```

Convert the object file to raw bytes:

```bash
riscv64-unknown-elf-objcopy -O binary program.o program.bin
```

Run the raw binary:

```bash
./simulator program.bin
```

## Creating `.hex` From a Binary

If you want the project-friendly one-word-per-line `.hex` format, convert the
raw bytes into little-endian 32-bit words. One common workflow is:

```bash
xxd -e -g4 -c4 program.bin | awk '{print $2}' > program.hex
```

Then run:

```bash
./simulator program.hex
```

## Why We Still Keep Hand-Written Examples

Hand-written `.hex` examples are small and deterministic. They are excellent
for learning hazards, forwarding, branch flushes, and memory latency because
every instruction is visible. Toolchain-produced binaries become more useful
when the simulator grows toward larger programs.
