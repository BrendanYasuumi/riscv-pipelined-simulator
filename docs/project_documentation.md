# RV32I Simulator Project Documentation

## Project Goal

This project is a C++17 RV32I CPU simulator with an assembly-first workflow.

The practical goal is:

```text
write a .s file
assemble it into machine code
run the generated binary on the simulator
inspect final registers and memory
```

The simulator does not parse assembly text directly. It models the CPU after a
real assembler has converted assembly into machine-code bytes.

## Current Workflow

```text
asmFiles/store_word.s
    -> RISC-V assembler
    -> build/asm/store_word.o
    -> RISC-V linker
    -> build/asm/store_word.elf
    -> objcopy
    -> build/asm/store_word.bin
    -> simulator memory
    -> final register/memory dump
```

Run it with:

```bash
make run ASM=asmFiles/store_word.s
```

## First Assembly Program

```asm
addi x4, x0, 24
addi x10, x0, 16
sw   x4, 0(x10)
ecall
```

Meaning:

```text
x4 = 24
x10 = 16
memory[x10 + 0] = x4
halt
```

Expected final state:

```text
x4 = 24
x10 = 16
memory[0x10] = 18 00 00 00
Halted: yes
```

`18 00 00 00` is the 32-bit value `24` stored in little-endian order.

## Design Scope

```text
assembly source workflow
raw binary program loading
register file
program counter
little-endian memory
five-stage pipeline
pipeline latches
instruction decoding
instruction execution
forwarding
load-use stalls
branches and jumps
halt
register dumps
memory dumps
automatic written-memory dumps
C++ unit tests
```

The simulator focuses on the path from a human-written assembly program to
final architectural state. Larger architecture experiments can be layered on
later, but the core project is intentionally organized around running RV32I
programs and inspecting the result.

## Important Architecture Concepts

### Program Counter

The PC stores the byte address of the next instruction to fetch. Normal
instructions advance the PC by 4 because RV32I instructions are 4 bytes wide.

Branches and jumps can redirect the PC to a different address.

### Registers

The CPU has 32 integer registers:

```text
x0 through x31
```

`x0` is special. It always reads as zero, and writes to it are ignored.

### Memory

Memory is a vector of bytes. A 32-bit word uses four adjacent bytes.

Little-endian means the least significant byte is stored at the lowest address.

Example:

```text
0x00000018 -> 18 00 00 00
```

The simulator also tracks memory addresses written during execution. With
`--dump-written-memory`, it prints those modified locations automatically, so
assembly examples do not all need to store results at the same address.

### Pipeline

The simulator models:

```text
IF -> ID -> EX -> MEM -> WB
```

Each cycle executes the C++ stage functions in reverse order:

```text
WB -> MEM -> EX -> ID -> IF
```

This models hardware latch timing. Every stage reads current latch values and
writes next latch values. At the end of the cycle, the next latches become the
current latches.

### Forwarding

Forwarding sends a result from a later pipeline stage directly back to the
execute stage. That lets dependent ALU instructions run without waiting for the
register file to be updated.

### Load-Use Stall

A load-use hazard happens when one instruction loads from memory and the very
next instruction needs that loaded value.

The loaded value is not ready early enough for normal forwarding, so the
pipeline inserts one bubble.

### Branches And Jumps

The simulator does not predict branches. It fetches the next
sequential instruction first. If EX later discovers that a branch or jump is
taken, the simulator redirects the PC and flushes younger wrong-path work.

## File Map

```text
include/config.hpp
    Small runtime configuration object.

include/cpu.hpp
src/cpu.cpp
    CPU architectural state: registers, PC, 64 KiB memory, halt flag, stats.

include/decoder.hpp
src/decoder.cpp
    Raw bit extraction for RISC-V instruction formats.

include/instruction.hpp
src/instruction.cpp
    Converts decoded fields into instruction kinds and control signals.

include/pipeline_registers.hpp
    Data carried between pipeline stages.

include/hazard_unit.hpp
src/hazard_unit.cpp
    Detects load-use hazards.

include/forwarding_unit.hpp
src/forwarding_unit.cpp
    Chooses forwarded operands for EX.

include/stages.hpp
src/stages.cpp
    Implements IF, ID, EX, MEM, WB, and the cycle runner.

include/program_loader.hpp
src/program_loader.cpp
    Loads raw binary bytes into memory.

include/state_dump.hpp
src/state_dump.cpp
    Prints final register state, manual memory windows, and written memory.

include/pipeline_trace.hpp
src/pipeline_trace.cpp
    Optional terminal trace of pipeline latches.

asmFiles/store_word.s
    First assembly program.

asmFiles/load_store.s
asmFiles/bitwise.s
asmFiles/branch_equal.s
asmFiles/count_loop.s
asmFiles/program1.s
asmFiles/program3.s
asmFiles/search.s
    Assembly programs used to practice memory, bitwise logic, branches, loops,
    stack usage, software multiplication, and search.

scripts/run_asm.sh
    Assembly build/run script.

linker/rv32i.ld
    Places the program at address 0x0.

tests/simulator_tests.cpp
    Unit tests for simulator internals.
```

## Interview Talking Point

The clean way to describe the project:

> I built a C++ RV32I simulator that runs real assembly programs by using a
> standard RISC-V assembler to produce a raw binary. The simulator loads that
> binary into simulated memory, starts at PC zero, advances a five-stage
> pipeline cycle by cycle, handles basic hazards, and prints final registers and
> memory so I can verify the program result.
