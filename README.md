# RV32I Pipelined CPU Simulator

A C++17 simulator for a small RV32I RISC-V CPU pipeline.

The project uses an assembly-first workflow: you write a `.s` RISC-V assembly
program, the build script assembles it into machine code, and the simulator
runs the resulting raw binary from simulated memory.

## Project Workflow

This repo is centered on one clear workflow:

```text
write assembly -> assemble/link -> create raw .bin -> run simulator -> inspect registers/memory
```

That keeps the project focused on CPU fundamentals and observable program
state.

## What The Simulator Models

- 32 RV32I integer registers
- `x0` hardwired to zero
- Program counter starting at address `0x0`
- Byte-addressable little-endian memory
- Five pipeline stages:
  - Instruction Fetch
  - Instruction Decode
  - Execute
  - Memory
  - Writeback
- Pipeline registers between stages
- Register forwarding for normal ALU dependencies
- Load-use stalls
- Branch and jump redirects
- Final register and memory dumps
- `ecall` / `ebreak` halt handling

## Supported Instructions

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

`nop` is encoded as:

```asm
addi x0, x0, 0
```

`halt` is represented with:

```asm
ecall
```

or:

```asm
ebreak
```

## Requirements

- C++17 compiler
- `make`
- RISC-V bare-metal binutils

On macOS with Homebrew:

```bash
brew install riscv64-elf-binutils
```

The script autodetects common prefixes such as `riscv64-elf-*` and
`riscv64-unknown-elf-*`.

## Build

```bash
make
```

If you see:

```text
make: Nothing to be done for 'all'
```

that means the simulator is already built and up to date.

## Run An Assembly Program

```bash
make run ASM=asmFiles/store_word.s
```

This runs:

```text
asmFiles/store_word.s
    -> build/asm/store_word.o
    -> build/asm/store_word.elf
    -> build/asm/store_word.bin
    -> ./simulator build/asm/store_word.bin --dump-regs --dump-written-memory
```

The first example program:

```asm
addi x4, x0, 24
addi x10, x0, 16
sw   x4, 0(x10)
ecall
```

Expected final result:

```text
x4 = 24
x10 = 16
memory[0x10] = 18 00 00 00
Halted: yes
```

`18 00 00 00` is decimal `24` stored as a 32-bit little-endian word.

## Run All Examples

```bash
make examples
```

`make examples` runs the assembly programs in `asmFiles/` and prints a compact
summary for each one:

```text
load_store    halted=yes cycles=11     retired=6      stalls=1
  writes: [0x00000040..0x00000044)
```

Full simulator output for each example is saved under:

```text
build/examples/
```

## Trace Pipeline Cycles

```bash
make trace ASM=asmFiles/load_store.s
```

`make trace` prints a bounded cycle-by-cycle view of the pipeline. By default
it prints the first 25 cycles so large programs do not flood the terminal.

Trace a later window:

```bash
make trace ASM=asmFiles/merge_sort.s TRACE_ARGS="--trace-start=100 --trace-limit=20"
```

This means: run `merge_sort.s`, but only print 20 trace entries starting at
cycle 100.

### Load-Use Hazard Demo

```bash
make trace ASM=asmFiles/hazard_demo.s TRACE_ARGS="--trace --trace-limit=12"
```

`hazard_demo.s` intentionally runs:

```asm
lw   x1, 0(x10)
add  x2, x1, x1
```

The `add` instruction needs `x1` immediately after `lw` loads it from memory.
Because load data is not ready soon enough for the next instruction's execute
stage, the hazard unit inserts one bubble. This is why the final stats include
a load-use stall cycle.

### ALU Forwarding Demo

```bash
make trace ASM=asmFiles/forwarding_demo.s TRACE_ARGS="--trace --trace-limit=10"
```

`forwarding_demo.s` intentionally runs two dependent ALU instructions:

```asm
addi x1, x0, 42
add  x2, x1, x1
```

When `add` reaches EX, `addi` has not written `x1` back to the register file.
The forwarding unit supplies the newer value directly from EX/MEM to both ALU
inputs, producing `84` without a stall. The program stores that result at
memory address `0x40`.

### Branch Control Hazard Demo

```bash
make trace ASM=asmFiles/branch_demo.s TRACE_ARGS="--trace --trace-limit=12"
```

`branch_demo.s` intentionally runs:

```asm
beq  x1, x2, taken
ecall
taken:
sw   x4, 0(x3)
```

The `ecall` after the branch is the wrong-path instruction. Since the branch is
taken, the simulator redirects the PC in the execute stage and flushes that
wrong-path instruction before it can retire.

## Run Tests

```bash
make test
```

`make test` runs C++ unit tests for the simulator internals. It checks things
like register behavior, memory reads/writes, instruction decoding, instruction
execution, forwarding, load-use stalls, branches, jumps, halt, and state dumps.

It does not run a user assembly program. Use `make run ASM=...` for that.

## Run Assembly Memory Tests

```bash
make asm-test
```

`make asm-test` runs real assembly programs through the full workflow:

```text
.s source -> assembler -> linker -> raw .bin -> simulator -> memory check
```

Each test verifies one or more final memory words with:

```bash
--expect-memory=ADDRESS:VALUE
```

Example:

```bash
./simulator build/asm/store_word.bin --expect-memory=0x10:24
```

This means: after the program finishes, the 32-bit word at memory address
`0x10` must equal decimal `24`.

## Useful Simulator Flags

Run a binary directly:

```bash
./simulator build/asm/store_word.bin --dump-regs --dump-written-memory
```

Print the full register file after execution:

```bash
--dump-regs
```

Print a memory window after execution:

```bash
--dump-memory=START:LENGTH
```

Example:

```bash
--dump-memory=0x10:4
```

Print every memory location written by the program:

```bash
--dump-written-memory
```

This is the easiest option while learning because you do not need to know in
advance where the program stored its result.

Check a final 32-bit memory value:

```bash
--expect-memory=ADDRESS:VALUE
```

Example:

```bash
--expect-memory=0x40:42
```

This is useful for automated tests because the simulator exits with failure if
the final memory value does not match.

Print cycle-by-cycle pipeline latch state:

```bash
--trace
```

Start tracing at a specific cycle:

```bash
--trace-start=100
```

Limit how many trace entries are printed:

```bash
--trace-limit=20
```

Limit execution if a program gets stuck:

```bash
--max-cycles=1000
```

## File Map

```text
asmFiles/
    Human-written RISC-V assembly programs.

scripts/run_asm.sh
    Converts a .s file into a raw .bin and runs the simulator.

scripts/asm_memory_tests.sh
    Runs assembly programs and checks expected final memory values.

scripts/run_examples.sh
    Runs all example assembly programs and prints a compact summary.

linker/rv32i.ld
    Places the assembled program at address 0x0.

include/cpu.hpp
src/cpu.cpp
    Registers, PC, memory, halt state, and execution stats.

include/decoder.hpp
src/decoder.cpp
    Extracts instruction bit fields.

include/instruction.hpp
src/instruction.cpp
    Classifies decoded instructions and creates control signals.

include/pipeline_registers.hpp
    Defines IF/ID, ID/EX, EX/MEM, and MEM/WB latches.

include/hazard_unit.hpp
src/hazard_unit.cpp
    Detects load-use hazards and requests stalls.

include/forwarding_unit.hpp
src/forwarding_unit.cpp
    Forwards newer values into the execute stage.

include/stages.hpp
src/stages.cpp
    Implements IF, ID, EX, MEM, WB, and one-cycle pipeline advancement.

include/program_loader.hpp
src/program_loader.cpp
    Loads raw .bin bytes into simulated memory.

include/state_dump.hpp
src/state_dump.cpp
    Prints final register and memory state.

include/pipeline_trace.hpp
src/pipeline_trace.cpp
    Prints optional cycle-by-cycle pipeline state.

tests/simulator_tests.cpp
    C++ unit tests for simulator behavior.
```
