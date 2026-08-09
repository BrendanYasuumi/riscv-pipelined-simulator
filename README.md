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
- CLI configuration flags
- Execution metrics
  - Total cycles
  - Instructions retired
  - CPI
  - IPC
  - Stall cycles
  - Branch mispredictions
- Assertion-based behavior tests

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

Set a max cycle limit:

```bash
./simulator examples/add.hex --max-cycles=500
```

Show help:

```bash
./simulator --help
```

## Hex Program Format

The simulator accepts simple `.hex` files with one 32-bit instruction word per
line.

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

## Example Output

Running:

```bash
./simulator examples/add.hex
```

Produces output similar to:

```text
Loaded hex program: examples/add.hex (3 instruction(s))

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

include/program_loader.hpp
src/program_loader.cpp
    External .hex program loader.

tests/simulator_tests.cpp
    Assertion-based behavior tests.

examples/add.hex
    Small sample machine-code program.

docs/
    Pseudocode, learning notes, and detailed project documentation.
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
- Forwarding behavior
- No-forwarding stall behavior
- Load-use hazard stall behavior
- Multi-cycle memory latency stalls
- Hex loader parsing
- Always-taken branch prediction behavior

## Current Limitations

- Supports a focused RV32I subset, not every RV32I instruction yet.
- Loads, stores, ALU operations, branches, jumps, and control signals are
  scaffolded, but more instruction-specific tests should be added.
- No ELF loader yet.
- No assembler yet.
- Branch predictor is intentionally small and direct-mapped.
- JALR target prediction cannot happen in IF because the target depends on a
  register value.

## Roadmap

- Add more RV32I instruction execution tests.
- Add raw binary program loading.
- Add more example programs.
- Add richer branch prediction tests.
- Add README diagrams or pipeline traces.
- Add optional per-cycle trace output.
- Add cache modeling hooks using the existing memory-latency interface.

## Design Notes

This project separates hardware concepts into focused modules:

- The CPU owns architectural state.
- The decoder extracts raw instruction fields.
- The instruction module generates control signals.
- Pipeline registers model hardware latches.
- The hazard unit decides when the pipeline must stall.
- The forwarding unit chooses bypassed operands.
- The stage module advances the simulated pipeline one cycle at a time.

That separation keeps the simulator easier to reason about and makes each
microarchitectural feature testable in isolation.
