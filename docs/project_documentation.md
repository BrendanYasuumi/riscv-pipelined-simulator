# Cycle-Accurate RV32I Pipelined Simulator Project Documentation

## Project Goal

This project is a C++17 cycle-accurate simulator for a five-stage RV32I
RISC-V pipeline. The goal is to model CPU behavior cycle by cycle instead of
executing each instruction instantly.

A cycle-accurate simulator tracks:

- Program counter updates
- Register file reads and writes
- Byte-addressable memory
- Pipeline latch movement
- Stalls and bubbles
- Retired instruction count
- Total cycle count
- CPI and IPC
- Branch mispredictions

The simulator is designed as a computer engineering portfolio project, so the
implementation favors clear architecture, modular files, and explicit hardware
rationale.

## Architecture Overview

The simulator currently models a classic five-stage pipeline:

```text
IF -> ID -> EX -> MEM -> WB
```

Each cycle executes the software stage functions in reverse order:

```text
WB -> MEM -> EX -> ID -> IF
```

This is intentional. In real hardware, all stages operate at the same time and
pipeline registers update together on the clock edge. Running the simulator in
reverse order helps each stage read the previous cycle's state and write the
next cycle's state.

## Step 1: CPU State and Configuration

### Files

```text
include/config.hpp
include/cpu.hpp
src/cpu.cpp
```

### What We Built

`Config` stores microarchitectural knobs:

```text
enable_forwarding
branch_predictor_type
memory_latency_cycles
enable_superscalar
cache_extension_ready
```

`CPU` stores architectural state:

```text
32 registers
program counter
byte-addressable memory
execution statistics
```

### Problem

The original project had a small CPU struct inside `main.cpp`. That was useful
for learning, but it was not modular enough for a real pipelined simulator.

### Solution

We moved CPU state into `include/cpu.hpp` and `src/cpu.cpp`.

### Why This Solution

Separating CPU state from the driver allows pipeline stages, decoders, tests,
and future tools to share the same CPU abstraction.

### Important Hardware Detail

Register `x0` is hardwired to zero:

```text
read x0 -> always returns 0
write x0 -> ignored
```

This is required by the RV32I ISA.

## Step 2: Little-Endian Memory

### Files

```text
include/cpu.hpp
src/cpu.cpp
```

### What We Built

The CPU supports:

```text
read_u8
read_u16
read_u32
write_u8
write_u16
write_u32
```

### Problem

RISC-V instructions are 32 bits wide, but memory is byte-addressable.

### Solution

We reconstruct 32-bit values from four bytes using little-endian order.

### Why This Solution

RV32I is little-endian in our simulator model. The lowest memory address stores
the least significant byte.

Example:

```text
memory[0] = 0xB3
memory[1] = 0x81
memory[2] = 0x20
memory[3] = 0x00

read_u32(0) = 0x002081B3
```

## Step 3: Instruction Field Decoder

### Files

```text
include/decoder.hpp
src/decoder.cpp
```

### What We Built

The decoder extracts fields for RV32I formats:

```text
R-type
I-type
S-type
B-type
U-type
J-type
```

### Problem

A 32-bit instruction is just raw bits. The simulator needs named fields like
`opcode`, `rd`, `rs1`, `rs2`, `funct3`, `funct7`, and `immediate`.

### Solution

We added bit extraction helpers and format-specific decode functions.

### Why This Solution

This mirrors hardware decoding. The instruction decoder should extract fields,
not execute instructions.

## Step 4: Pipeline Registers

### Files

```text
include/pipeline_registers.hpp
```

### What We Built

Pipeline latch structs:

```text
IFIDRegister
IDEXRegister
EXMEMRegister
MEMWBRegister
PipelineRegisters
```

### Problem

Without pipeline registers, an instruction could incorrectly move through every
stage in one software loop.

### Solution

We added current and next versions of each pipeline latch:

```text
if_id      next_if_id
id_ex      next_id_ex
ex_mem     next_ex_mem
mem_wb     next_mem_wb
```

### Why This Solution

The current latches represent the state before the clock edge. The next latches
represent what will be captured at the clock edge.

Pseudocode:

```text
commit:
    IF/ID  = next IF/ID
    ID/EX  = next ID/EX
    EX/MEM = next EX/MEM
    MEM/WB = next MEM/WB
```

### Important Hardware Detail

Each latch has a `valid` bit:

```text
valid = true  -> real instruction
valid = false -> bubble / empty slot
```

This is necessary for stalls, flushes, and pipeline drain.

## Step 5: Instruction Classification and Control Signals

### Files

```text
include/instruction.hpp
src/instruction.cpp
```

### What We Built

Instruction classification converts raw fields into an instruction meaning:

```text
Add
Sub
Addi
Lw
Sw
Beq
Jal
Invalid
...
```

It also generates control signals:

```text
reg_write
mem_read
mem_write
branch
jump
alu_src_pc
alu_src_imm
alu_op
writeback_source
memory_width
branch_condition
```

### Problem

The field decoder can tell us `opcode = 0x33`, but the pipeline needs to know
whether the instruction is `add`, `sub`, `lw`, `sw`, or a branch.

### Solution

We added a control-unit layer that classifies instructions and generates
latched control signals.

### Why This Solution

Later stages should not repeatedly decode raw bits. Decode should decide the
instruction's intent once, then pass control signals through the pipeline.

## Step 6: Five-Stage Pipeline Skeleton

### Files

```text
include/stages.hpp
src/stages.cpp
```

### What We Built

Stage functions:

```text
stage_IF
stage_ID
stage_EX
stage_MEM
stage_WB
run_pipeline_cycle
```

### Problem

The project had modules, but no actual cycle-by-cycle pipeline behavior.

### Solution

We implemented a pipeline cycle function:

```text
run_pipeline_cycle:
    clear next latches
    run WB
    run MEM
    run EX
    run ID
    run IF
    commit latches
    tick clock
```

### Why This Solution

This models simultaneous hardware stage behavior while still using ordinary
sequential C++ code.

## Step 7: Simulator Driver and Build System

### Files

```text
main.cpp
Makefile
docs/learning_notes.md
```

### What We Built

`main.cpp` now:

```text
creates CPU
creates pipeline registers
loads demo machine code into RAM
runs pipeline cycles
prints registers and stats
```

The `Makefile` supports:

```text
make
make run
make clean
```

### Problem

The codebase needed a repeatable way to build and run the simulator.

### Solution

We replaced the scratch `main.cpp` and added a small build system.

### Why This Solution

A portfolio project should be easy for an interviewer to build and run with one
or two commands.

## Step 8: RAW Hazard Detection and Stalls

### Files

```text
include/hazard_unit.hpp
src/hazard_unit.cpp
src/stages.cpp
main.cpp
```

### What We Built

The hazard unit detects read-after-write dependencies.

### Problem

This instruction sequence has dependencies:

```text
addi x1, x0, 5
addi x2, x0, 7
add  x3, x1, x2
```

The `add` reads `x1` and `x2`, but the earlier `addi` instructions may not
have written those registers yet.

### Solution

If the instruction in decode needs a register that an older instruction in
`ID/EX` or `EX/MEM` will write, the simulator stalls.

Pseudocode:

```text
if RAW hazard:
    keep IF/ID unchanged
    keep PC unchanged
    insert bubble into ID/EX
    stall_cycles += 1
```

### Why This Solution

Stalling is the simplest correct solution before forwarding exists. It makes
the simulator correct even when the assembly program does not manually include
`nop` instructions.

### Why We Do Not Stall on MEM/WB

The simulator runs writeback before decode in each cycle:

```text
WB -> MEM -> EX -> ID -> IF
```

So a value in `MEM/WB` writes the register file before decode reads registers
in that same cycle.

## Step 9: Forwarding Support

### Files

```text
include/forwarding_unit.hpp
src/forwarding_unit.cpp
include/hazard_unit.hpp
src/hazard_unit.cpp
src/stages.cpp
main.cpp
```

### What We Built

The forwarding unit resolves operand values for the EX stage.

Supported forwarding paths:

```text
EX/MEM -> EX
MEM/WB -> EX
```

### Problem

RAW stalls are correct, but they are conservative. Many ALU results are already
available before writeback.

Example:

```text
addi x1, x0, 5
add  x2, x1, x0
```

The `add` needs `x1`, but the `addi` result can be forwarded from a later
pipeline stage instead of waiting for register writeback.

### Solution

Before EX chooses ALU operands, the forwarding unit checks whether `rs1` or
`rs2` matches a destination register in a later pipeline stage.

Pseudocode:

```text
resolve_forwarding:
    start with the values read during decode

    if EX/MEM writes a matching rd:
        use EX/MEM result

    else if MEM/WB writes a matching rd:
        use MEM/WB writeback value
```

### Why This Solution

Forwarding improves performance without changing architectural behavior. The
same final register values are produced, but fewer bubbles are inserted.

### Why EX/MEM Has Priority

If both `EX/MEM` and `MEM/WB` match the same source register, `EX/MEM` is newer.
The simulator forwards the newest available value.

### Why Load-Use Still Stalls

Loads produce their value in MEM, not EX. If an instruction immediately after a
load needs the loaded value, the data is not available early enough for a normal
EX/MEM bypass.

Example:

```text
lw   x1, 0(x2)
add  x3, x1, x4
```

The hazard unit still inserts one bubble for this case when forwarding is
enabled.

## Step 10: Tests, Hex Loader, CLI, Branch Prediction, and Memory Latency

### Files

```text
include/program_loader.hpp
src/program_loader.cpp
tests/simulator_tests.cpp
examples/add.hex
main.cpp
Makefile
include/cpu.hpp
src/cpu.cpp
include/pipeline_registers.hpp
src/stages.cpp
```

### What We Built

This step made the simulator runnable as a small program runner instead of only
a hardcoded demo.

New capabilities:

```text
make test
external .hex program loading
CLI configuration flags
always-not-taken prediction
always-taken prediction
2-bit saturating counter prediction
multi-cycle memory stalls
```

### Problem

The simulator had useful internal modules, but it still needed:

```text
repeatable tests
a way to load programs without editing C++
runtime configuration knobs
control-hazard experimentation
memory-latency experimentation
```

### Solution

We added an assertion-based test executable and a simple hex loader. We also
expanded the CLI so architectural experiments can be run from the terminal.

Example commands:

```text
make
make test
./simulator
./simulator examples/add.hex
./simulator examples/add.hex --no-forwarding
./simulator examples/add.hex --memory-latency=3
./simulator examples/add.hex --branch-predictor=two-bit
```

### Why This Solution

A plain hex loader is much simpler than ELF support and is enough to run small
machine-code programs. It also keeps the project focused on pipeline timing
rather than file-format complexity.

### Branch Prediction Rationale

Branches create control hazards because fetch needs the next PC before EX knows
whether the branch is actually taken.

The simulator now stores prediction metadata in the pipeline:

```text
predicted_taken
predicted_target
```

EX compares this prediction against the actual branch result. If they differ,
the simulator redirects the PC and flushes younger work.

### Memory Latency Rationale

`memory_latency_cycles` now affects load/store timing. If memory takes multiple
cycles, MEM holds the operation and freezes younger stages until the access is
ready to complete.

### Test Coverage

The test runner currently checks:

```text
x0 hardwired behavior
little-endian memory
decoder field extraction
instruction classification
forwarding behavior
no-forwarding stall behavior
load-use stalls
memory-latency stalls
hex loader parsing
always-taken branch prediction
```

## Step 11: RV32I Execution Tests and Pipeline Trace Output

### Files

```text
tests/simulator_tests.cpp
include/pipeline_trace.hpp
src/pipeline_trace.cpp
main.cpp
README.md
```

### What We Built

We expanded the test suite from infrastructure checks into instruction execution
coverage. We also added a `--trace` option that prints the contents of each
pipeline latch at the start of every cycle.

### Problem

The simulator could decode and run simple programs, but more instruction
families needed explicit regression coverage. Also, final register values alone
do not show how instructions moved through the pipeline.

### Solution

We added test-local instruction encoders and wrote execution tests for:

```text
R-type ALU operations
I-type ALU operations
loads and stores
signed and unsigned loads
LUI
AUIPC
BEQ/BNE behavior
JAL
JALR
```

We added trace output:

```text
./simulator examples/add.hex --trace
```

### Why This Solution

The tests protect architectural correctness. The trace output helps explain
cycle-by-cycle timing, which is valuable for debugging and for interviews.

### What `.hex` Programs Are

A `.hex` file is a plain text machine-code program. Each non-comment line is a
single 32-bit instruction word:

```text
00500093
00700113
002081b3
```

This is not assembly. It is already-encoded instruction data. The loader turns
each hex word into four little-endian bytes in simulated memory.

## Current Demo Program

The current demo program is:

```text
addi x1, x0, 5
addi x2, x0, 7
add  x3, x1, x2
```

Expected result:

```text
x1 = 5
x2 = 7
x3 = 12
stall_cycles = 0 when forwarding is enabled
```

## Current File Map

```text
include/config.hpp
    Microarchitectural configuration knobs.

include/cpu.hpp
src/cpu.cpp
    CPU architectural state, memory, PC, registers, and stats.

include/decoder.hpp
src/decoder.cpp
    Raw instruction bit-field extraction.

include/instruction.hpp
src/instruction.cpp
    Instruction classification and control-signal generation.

include/pipeline_registers.hpp
    Inter-stage hardware latches and control-signal types.

include/hazard_unit.hpp
src/hazard_unit.cpp
    RAW hazard detection and source-register analysis.

include/forwarding_unit.hpp
src/forwarding_unit.cpp
    EX-stage operand forwarding from EX/MEM and MEM/WB.

include/stages.hpp
src/stages.cpp
    Five pipeline stages and the cycle runner.

include/pipeline_trace.hpp
src/pipeline_trace.cpp
    Per-cycle pipeline latch trace output.

include/program_loader.hpp
src/program_loader.cpp
    Hex program loading.

tests/simulator_tests.cpp
    Assertion-based simulator behavior tests.

examples/add.hex
    Small external hex program for the default add dependency example.

main.cpp
    Demo simulator entry point.

Makefile
    Build and run commands.

docs/pseudocode.md
    Architecture-level pseudocode.

docs/learning_notes.md
    Learning notes for each implementation milestone.
```

## Known Limitations

- No external ELF or binary loader yet.
- No assembler support yet.
- Branch predictor is small and direct-mapped.
- JALR cannot be predicted in IF because its target depends on a register value.

## Recommended Next Steps

1. Add a binary loader for raw `.bin` files.
2. Add richer branch predictor tests.
3. Add simple assembly-to-hex examples in `examples/`.
4. Add README diagrams.
5. Add cache modeling hooks.
