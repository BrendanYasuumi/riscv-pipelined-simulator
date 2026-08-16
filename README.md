# Cycle-Accurate RV32I Pipelined Simulator

A C++17 cycle-accurate simulator for a five-stage RV32I RISC-V pipeline.

This project models CPU execution cycle by cycle instead of treating
instructions as instantaneous operations. It is designed as a computer
engineering portfolio project focused on pipeline timing, hazards, forwarding,
branch prediction, memory latency, and performance metrics.

## Features

- RV32I-style architectural state
  - 32 general-purpose 32-bit registers
  - `x0` hardwired to zero
  - Program counter
  - Byte-addressable little-endian memory
- Five-stage pipeline model
  - Instruction Fetch
  - Instruction Decode
  - Execute
  - Memory
  - Writeback
- Inter-stage pipeline registers
  - `IF/ID`
  - `ID/EX`
  - `EX/MEM`
  - `MEM/WB`
- RAW hazard detection
- Pipeline stalls and bubbles
- Forwarding support
  - `EX/MEM -> EX`
  - `MEM/WB -> EX`
- Load-use stall handling
- Configurable branch prediction
  - Always not taken
  - Always taken
  - 2-bit saturating counter
- Configurable memory latency
- External `.hex` program loader
- Raw `.bin` program loader
- CLI configuration flags
- Optional per-cycle pipeline trace output
- Optional CSV trace output
- Optional full register and memory window dumps
- `ecall`/`ebreak` halt handling
- Execution metrics
  - Total cycles
  - Instructions retired
  - CPI
  - IPC
  - Stall cycles
  - Branch mispredictions
- Assertion-based behavior tests
- Golden-output regression test using `diff`

## Supported Instruction Subset

The simulator functionally executes this RV32I subset:

```text
add   addi   and   andi
beq   bge    bgeu  blt   bltu  bne
jal   jalr   lui
lw    sw
or    ori
sll   slli
xor   xori
nop   halt
```

`nop` is implemented using the normal RISC-V encoding:

```text
addi x0, x0, 0
```

`halt` is modeled with system instructions:

```text
ecall
ebreak
```

When the decode stage sees a halt instruction, fetch stops. When halt reaches
writeback, the CPU marks the simulation as halted.

## Pipeline Model

The simulator models a classic five-stage pipeline:

```text
IF -> ID -> EX -> MEM -> WB
```

Each simulated cycle executes stage functions in reverse order:

```text
WB -> MEM -> EX -> ID -> IF
```

This is intentional. In real hardware, pipeline registers update together on a
clock edge. Running stages in reverse order lets each stage read the current
cycle's latch values and write next-cycle latch values without letting one
instruction pass through multiple stages in a single software iteration.

Simplified cycle pseudocode:

```text
run_pipeline_cycle:
    clear next pipeline registers

    stage_WB
    stage_MEM
    stage_EX
    hazard detection
    stage_ID
    stage_IF

    commit next pipeline registers
    clock_cycles += 1
```

## Architecture Diagrams

High-level datapath:

```text
        instruction word        decoded control        ALU result        memory data
PC -> IF --------------> IF/ID -> ID ----------> ID/EX -> EX ----> EX/MEM -> MEM ----> MEM/WB -> WB
^                                                                                              |
|                                                                                              |
+--------------------------------------- next PC / branch redirect ----------------------------+
```

Pipeline latches:

```text
IF/ID:  fetched instruction + fetch PC
ID/EX:  decoded fields + register operands + control signals
EX/MEM: ALU result + store value + branch decision + memory controls
MEM/WB: load data or ALU result + destination register + writeback controls
```

Forwarding paths:

```text
EX/MEM result ----+
                 +----> EX operand muxes
MEM/WB result ----+
```

Load-use stall:

```text
Cycle N:
    load is in ID/EX
    dependent consumer is in IF/ID

Action:
    freeze PC
    freeze IF/ID
    insert bubble into ID/EX
```

Branch misprediction recovery:

```text
EX resolves branch
if predicted next PC was wrong:
    redirect PC to actual target
    flush younger wrong-path latch work
    branch_mispredictions += 1
```

## Build

Requires a C++17 compiler and `make`.

```bash
make
```

Run the built-in demo:

```bash
make run
```

Run tests:

```bash
make test
```

Run the golden-output regression:

```bash
make regression
```

Clean build outputs:

```bash
make clean
```

## Usage

Run the built-in demo program:

```bash
./simulator
```

Run an external hex program:

```bash
./simulator examples/add.hex
```

Run a raw binary program:

```bash
./simulator program.bin
./simulator --format=bin program.bin
```

Disable forwarding:

```bash
./simulator examples/add.hex --no-forwarding
```

Set memory latency:

```bash
./simulator examples/add.hex --memory-latency=3
```

Select branch predictor:

```bash
./simulator examples/add.hex --branch-predictor=always-not-taken
./simulator examples/add.hex --branch-predictor=always-taken
./simulator examples/add.hex --branch-predictor=two-bit
```

Export a CSV pipeline trace:

```bash
./simulator examples/add.hex --trace-csv=trace.csv
```

Dump all architectural registers after simulation:

```bash
./simulator examples/add.hex --dump-regs
```

Dump a memory window after simulation:

```bash
./simulator examples/store_load.hex --dump-memory=64:16
./simulator examples/store_load.hex --dump-memory=0x40:0x10
```

Set a max cycle limit:

```bash
./simulator examples/add.hex --max-cycles=500
```

Override the number of instructions to retire before stopping:

```bash
./simulator examples/branch_taken.hex --retire-count=2
```

Print a per-cycle pipeline trace:

```bash
./simulator examples/add.hex --trace
```

Show help:

```bash
./simulator --help
```

## Hex Program Format

The simulator accepts simple `.hex` files with one 32-bit instruction word per
line.

A `.hex` program is not assembly source. It is a text representation of the
machine code that the CPU fetches. Each line is one instruction already encoded
as a 32-bit hexadecimal value.

Example:

```text
# addi x1, x0, 5
00500093

# addi x2, x0, 7
00700113

# add x3, x1, x2
002081b3
```

Comments begin with `#`. Words may optionally use a `0x` prefix.

The loader stores each 32-bit word into memory in little-endian byte order.
For example, the line `00500093` represents the instruction word
`0x00500093`, which the loader places into byte-addressable RAM as:

```text
93 00 50 00
```

For a deeper walkthrough of how assembly maps to these 32-bit words, see
[`docs/instruction_encoding_guide.md`](docs/instruction_encoding_guide.md).

## Binary Program Format

Raw `.bin` programs contain the bytes exactly as they should appear in
simulated memory. The byte count must be a multiple of four because this
simulator currently treats each loaded word as one 32-bit RV32I instruction.

Format selection is automatic for `.hex` and `.bin` paths, or explicit:

```bash
./simulator --format=hex examples/add.hex
./simulator --format=bin program.bin
```

For notes on assembling RV32I source with an external RISC-V toolchain, see
[`docs/toolchain_workflow.md`](docs/toolchain_workflow.md).

## Runnable Examples

Basic ALU dependency with forwarding:

```bash
./simulator examples/add.hex
```

Load-use hazard:

```bash
./simulator examples/load_use.hex
```

Taken branch with trace output:

```bash
./simulator examples/branch_taken.hex --retire-count=2 --trace
```

Multi-cycle memory latency:

```bash
./simulator examples/memory_latency.hex --memory-latency=4
```

Forwarding vs. no forwarding:

```bash
./simulator examples/no_forwarding_demo.hex
./simulator examples/no_forwarding_demo.hex --no-forwarding
```

Store followed by load:

```bash
./simulator examples/store_load.hex --dump-regs --dump-memory=64:16
```

JAL control-flow redirect:

```bash
./simulator examples/jump.hex --retire-count=2 --trace
```

Branch predictor comparison:

```bash
./simulator examples/branch_predictor.hex --retire-count=2
./simulator examples/branch_predictor.hex --retire-count=2 --branch-predictor=always-taken
```

Shift and comparison operations:

```bash
./simulator examples/shift_compare.hex --dump-regs
```

Required instruction subset regression:

```bash
./simulator examples/required_subset.hex --dump-regs --dump-memory=128:4
make regression
```

## Example Output

Running:

```bash
./simulator examples/add.hex
```

Produces output similar to:

```text
Loaded program: examples/add.hex (3 instruction(s))

Configuration:
  Forwarding:       enabled
  Memory latency:   1 cycle(s)
  Branch predictor: always-not-taken
  Max cycles:       100

Architectural registers:
  x0 = 0
  x1 = 5
  x2 = 7
  x3 = 12

Execution stats:
  Total cycles:          7
  Instructions retired:  3
  CPI:                   2.33333
  IPC:                   0.428571
  Stall cycles:          0
  Branch mispredictions: 0
```

With forwarding disabled:

```bash
./simulator examples/add.hex --no-forwarding
```

The same program still produces the correct result, but inserts dependency
stalls:

```text
Stall cycles: 2
```

## File Structure

```text
include/config.hpp
    Microarchitectural configuration knobs.

include/cpu.hpp
src/cpu.cpp
    CPU architectural state: registers, PC, memory, branch predictor state,
    and execution statistics.

include/decoder.hpp
src/decoder.cpp
    Raw RV32I instruction field extraction for R/I/S/B/U/J formats.

include/instruction.hpp
src/instruction.cpp
    Instruction classification and control-signal generation.

include/pipeline_registers.hpp
    Inter-stage hardware latch definitions.

include/hazard_unit.hpp
src/hazard_unit.cpp
    RAW dependency detection and stall decisions.

include/forwarding_unit.hpp
src/forwarding_unit.cpp
    EX-stage operand forwarding from later pipeline stages.

include/stages.hpp
src/stages.cpp
    Five pipeline stages and the cycle runner.

include/pipeline_trace.hpp
src/pipeline_trace.cpp
    Optional per-cycle text and CSV pipeline latch tracing.

include/program_loader.hpp
src/program_loader.cpp
    External .hex and raw .bin program loaders.

include/state_dump.hpp
src/state_dump.cpp
    Register and memory dump formatting helpers.

tests/simulator_tests.cpp
    Assertion-based behavior tests.

examples/add.hex
examples/load_use.hex
examples/branch_taken.hex
examples/memory_latency.hex
examples/no_forwarding_demo.hex
examples/store_load.hex
examples/jump.hex
examples/branch_predictor.hex
examples/shift_compare.hex
examples/required_subset.hex
    Small sample machine-code programs for architecture experiments.

docs/
    Pseudocode, learning notes, and detailed project documentation.
    Includes an RV32I instruction encoding guide and toolchain workflow notes.
```

## Tests

Run:

```bash
make test
```

Current tests cover:

- `x0` hardwired behavior
- Little-endian memory access
- R-type instruction field decoding
- Instruction classification
- R-type ALU execution
- I-type ALU execution
- Load/store execution
- LUI/AUIPC execution
- Branch flush behavior
- JAL/JALR behavior
- Forwarding behavior
- No-forwarding stall behavior
- Load-use hazard stall behavior
- Multi-cycle memory latency stalls
- Hex loader parsing
- Binary loader parsing
- Always-taken branch prediction behavior
- Always-taken misprediction behavior
- 2-bit branch predictor counter behavior
- CSV trace formatting
- Register dump formatting
- Memory dump formatting
- Halt behavior
- Required instruction subset execution
- Golden-output CLI regression with `diff`

## Current Limitations

- Supports a focused RV32I subset, not every RV32I instruction yet.
- No assembler yet.
- No ELF loader yet.
- Branch predictor is intentionally small and direct-mapped.
- JALR target prediction cannot happen in IF because the target depends on a
  register value.

## Roadmap

- Add cache modeling hooks using the existing memory-latency interface.
- Add an ELF loader or scripted integration with external RISC-V toolchains.
- Add GitHub Actions CI for `make test`.

## Design Notes

This project separates hardware concepts into focused modules:

- The CPU owns architectural state.
- The decoder extracts raw instruction fields.
- The instruction module generates control signals.
- Pipeline registers model hardware latches.
- The hazard unit decides when the pipeline must stall.
- The forwarding unit chooses bypassed operands.
- The stage module advances the simulated pipeline one cycle at a time.
- The trace module prints latch contents so timing behavior can be inspected.

That separation keeps the simulator easier to reason about and makes each
microarchitectural feature testable in isolation.
