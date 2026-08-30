# Running Assembly Programs

The simulator executes machine code, not assembly text. To run assembly, use an
external RISC-V assembler to translate `.s` files into raw binary bytes, then
load that binary into the simulator.

## First Runnable Assembly Program

The first assembly example is:

```text
asmFiles/store_word.s
```

It does this:

```text
x4 = 24
x10 = 16
memory[0x10] = x4
halt
```

Expected final state:

```text
x4 = 24
x10 = 16
memory[0x10] = 24
Halted: yes
```

## Run With Make

```bash
make run ASM=asmFiles/store_word.s
```

This calls:

```text
scripts/run_asm.sh
```

The Makefile is the easy shortcut. The script contains the actual assembly,
link, objcopy, and simulator commands.

## Run The Script Directly

```bash
./scripts/run_asm.sh asmFiles/store_word.s --dump-regs --dump-written-memory
```

## What The Script Does

```text
asmFiles/store_word.s
    -> build/asm/store_word.o
    -> build/asm/store_word.elf
    -> build/asm/store_word.bin
    -> ./simulator build/asm/store_word.bin
```

The default `make run` command uses `--dump-written-memory`, so the simulator
prints the memory addresses that the program actually changed.

## Required External Tools

The script expects a bare-metal RISC-V GNU toolchain on PATH:

```text
riscv64-unknown-elf-as
riscv64-unknown-elf-ld
riscv64-unknown-elf-objcopy
```

On Homebrew, the package may be:

```bash
brew install riscv64-elf-binutils
```

That package commonly uses the `riscv64-elf-*` command prefix. The script
autodetects both `riscv64-unknown-elf-*` and `riscv64-elf-*`.

If your installed toolchain uses a different prefix, set:

```bash
RISCV_TOOL_PREFIX=riscv64-elf ./scripts/run_asm.sh asmFiles/store_word.s --dump-regs
```

## Why The Linker Script Exists

The simulator loads the raw binary at address `0x0` and starts `PC` at `0x0`.
The linker script:

```text
linker/rv32i.ld
```

places the program at address `0x00000000`, which matches the simulator's
startup model.
