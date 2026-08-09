# Learning Notes

## Simulator Driver and Build System

### What changed

- Replaced the old scratch `main.cpp` with a real simulator driver.
- Added a `Makefile` so the project can be built with `make`.
- Loaded a small RV32I demo program directly into simulated RAM.
- Ran enough cycles to let the five-stage pipeline drain.
- Printed architectural register results and performance stats.

### Why `main.cpp` matters now

Before this step, the project had good modules, but no official entry point that
connected them. The new driver creates the CPU, creates the pipeline latches,
loads instructions into memory, runs clock cycles, and prints the final state.

Pseudocode:

```text
main:
    config = default configuration
    cpu = CPU(1024 bytes of RAM, config)
    pipeline = empty pipeline registers

    load demo program into RAM at address 0
    PC = 0

    repeat program_length + 4 cycles:
        run_pipeline_cycle(cpu, pipeline)

    print registers
    print execution stats
```

### Why the demo program has NOPs

The demo program is:

```text
addi x1, x0, 5
addi x2, x0, 7
nop
nop
add  x3, x1, x2
```

The two `nop` instructions are temporary spacing. We do not have hazard
detection or forwarding yet, so the `add` must wait until the earlier `addi`
instructions write their results back to the register file.

Without those `nop`s, `add x3, x1, x2` could read old register values.

### What a NOP is

RISC-V usually represents `nop` as:

```text
addi x0, x0, 0
```

Its machine code is:

```text
0x00000013
```

It flows through the pipeline like a normal instruction, but it does not change
architectural state because writes to `x0` are ignored.

### Why run `instruction_count + 4` cycles

A five-stage pipeline has these stages:

```text
IF -> ID -> EX -> MEM -> WB
```

The final instruction still needs four extra cycles after it is fetched to move
through decode, execute, memory, and writeback. This is called draining the
pipeline.

### Useful commands

```text
make
make run
make clean
```

## RAW Hazard Detection and Stalls

### What changed

- Added a hazard unit that detects read-after-write dependencies.
- Updated the pipeline cycle so a hazard freezes fetch/decode and inserts a
  bubble into execute.
- Removed manual `nop` spacing from the demo program.

### What problem this solves

This program has a dependency:

```text
addi x1, x0, 5
addi x2, x0, 7
add  x3, x1, x2
```

The `add` reads `x1` and `x2`, but those values are produced by older
instructions that may still be moving through the pipeline.

This is called a RAW hazard:

```text
Read After Write:
    a younger instruction reads a register
    before an older instruction has written it
```

### Why the solution stalls

The conservative first solution is to wait until the older instruction has
written back to the register file.

Pseudocode:

```text
detect_raw_hazard:
    decode instruction in IF/ID
    find which source registers it reads

    if ID/EX will write a matching destination register:
        stall

    if EX/MEM will write a matching destination register:
        stall

    otherwise:
        continue normally
```

When a stall happens:

```text
next IF/ID = current IF/ID
PC does not advance
next ID/EX = bubble
stall_cycles += 1
```

### Why MEM/WB is not included

In this simulator cycle order, writeback runs before decode:

```text
WB -> MEM -> EX -> ID -> IF
```

That means an instruction in `MEM/WB` writes the register file before the decode
stage reads it during the same simulated cycle. So the hazard unit only needs
to stall on producers still in `ID/EX` or `EX/MEM`.

### What this teaches

Stalling is correct but slow. It protects correctness by inserting bubbles.
The next performance improvement is forwarding, where the CPU sends a result
directly from a later stage to execute instead of waiting for writeback.
