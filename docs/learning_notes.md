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
