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

    repeat until the demo instructions retire:
        run_pipeline_cycle(cpu, pipeline)

    print registers
    print execution stats
```

### Why the first demo program had NOPs

The first pipeline demo program was:

```text
addi x1, x0, 5
addi x2, x0, 7
nop
nop
add  x3, x1, x2
```

The two `nop` instructions were temporary spacing. At that moment, we did not
have hazard detection or forwarding yet, so the `add` had to wait until the
earlier `addi` instructions wrote their results back to the register file.

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

## Forwarding Support

### What changed

- Added a forwarding unit that can replace stale `rs1` and `rs2` values in EX.
- Updated hazard detection so `enable_forwarding = true` avoids unnecessary ALU
  dependency stalls.
- Kept one-cycle load-use stalls because load data is not available soon enough
  for the immediately following EX stage.
- Updated `main.cpp` to run until the demo instructions retire instead of using
  a fixed cycle count.

### What problem this solves

Stalling is correct, but it loses cycles even when the needed value already
exists somewhere in the pipeline.

Example:

```text
addi x1, x0, 5
add  x2, x1, x0
```

The `addi` computes `x1` in EX before it reaches WB. Forwarding lets the next
instruction use that computed value directly from a later pipeline stage.

### Forwarding paths

The simulator now supports:

```text
EX/MEM -> EX
MEM/WB -> EX
```

Meaning:

```text
if instruction in EX needs rs1 or rs2:
    first check whether EX/MEM will write that register
    otherwise check whether MEM/WB will write that register
    use the newest matching value
```

### Why EX/MEM has priority

If both later stages refer to the same register, the value in `EX/MEM` is newer
than the value in `MEM/WB`.

Pseudocode:

```text
resolve_forwarding:
    rs1_value = ID/EX.rs1_value
    rs2_value = ID/EX.rs2_value

    if EX/MEM can forward rd:
        if rd == rs1: rs1_value = EX/MEM result
        if rd == rs2: rs2_value = EX/MEM result

    if MEM/WB can forward rd and source was not already forwarded:
        if rd == rs1: rs1_value = MEM/WB result
        if rd == rs2: rs2_value = MEM/WB result
```

### Why load-use still stalls

A load gets its data in MEM, not EX. If the very next instruction needs that
loaded register in EX, the data is still arriving too late for a normal EX/MEM
forwarding path.

Example:

```text
lw   x1, 0(x2)
add  x3, x1, x4
```

The simulator still inserts a bubble for this case.

### What this teaches

Forwarding improves performance without changing the program's architectural
result. The same instructions retire with the same final register values, but
fewer cycles are wasted on bubbles.

## Tests, Hex Loader, CLI, Branch Prediction, and Memory Latency

### What changed

- Added `make test` with assertion-based simulator behavior tests.
- Added a hex loader so programs can be loaded from files.
- Added CLI flags for forwarding, memory latency, branch predictor policy, and
  max cycle count.
- Added branch prediction metadata to the fetch/decode path.
- Added CPU-owned 2-bit branch predictor counters.
- Added memory-stage stalls for multi-cycle load/store latency.

### Why tests came first

Pipeline simulators are easy to break because a timing change in one stage can
silently affect another stage. Tests give us a quick way to check important
hardware rules after every change.

Current tests cover:

```text
x0 stays zero
little-endian memory
R-type field decoding
instruction classification
forwarding removes ALU stalls
disabling forwarding restores stalls
load-use still stalls once
memory latency adds stalls
hex loader parsing
always-taken branch prediction
```

### Hex program format

A `.hex` file contains one 32-bit instruction word per line:

```text
00500093
00700113
002081b3
```

The loader stores each word into memory in little-endian byte order.

### CLI examples

```text
./simulator
./simulator examples/add.hex
./simulator examples/add.hex --no-forwarding
./simulator examples/add.hex --memory-latency=3
./simulator examples/add.hex --branch-predictor=two-bit
./simulator examples/add.hex --max-cycles=500
```

### Branch prediction model

The fetch stage predicts branches before EX knows the true result.

Supported policies:

```text
always-not-taken
always-taken
two-bit
```

For the 2-bit predictor:

```text
0 = strongly not taken
1 = weakly not taken
2 = weakly taken
3 = strongly taken
```

When a branch resolves in EX:

```text
if prediction was wrong:
    redirect PC
    flush younger decode work
    branch_mispredictions += 1

if using two-bit predictor:
    update saturating counter
```

### Memory latency model

`memory_latency_cycles = 1` means ideal single-cycle memory.

If memory latency is greater than one:

```text
MEM holds the load/store
younger stages are frozen
stall_cycles += 1 each waiting cycle
```

This models a slow RAM access or a future cache miss path.

### What this teaches

At this point the simulator can run small external machine-code programs. It is
still not an ELF loader or assembler, but it now has the core pieces needed for
architectural experiments:

```text
program loading
pipeline timing
hazards
forwarding
branch prediction
memory latency
metrics
tests
```
