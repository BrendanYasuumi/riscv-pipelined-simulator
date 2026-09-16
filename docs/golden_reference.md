# Spike Golden Reference

The golden-reference workflow executes the same relocated RV32I program on
two independent models:

```text
assembly source
      |
      v
one ELF and raw binary
      |
      +-------------------+
      |                   |
      v                   v
pipelined simulator      Spike
      |                   |
      v                   v
simulator-state.json   spike-state.json
      |                   |
      +------ exact diff -+
```

Spike is the independent functional reference. It does not validate cycle
counts or pipeline timing; it validates final architectural behavior.

## Install Spike On macOS

```bash
brew tap riscv-software-src/riscv
brew install riscv-software-src/riscv/riscv-isa-sim
```

Verify the installation:

```bash
spike --help
```

## Run Golden Tests

```bash
make golden
```

Successful output looks like:

```text
Spike golden test: store_word             PASS (diff = 0)
Spike golden test: instruction_coverage   PASS (diff = 0)
Spike golden test: control_flow_coverage  PASS (diff = 0)
Spike golden test: add                    PASS (diff = 0)
Spike golden test: arithmetic             PASS (diff = 0)
Spike golden test: bitwise                PASS (diff = 0)
Spike golden test: shifts                 PASS (diff = 0)
Spike golden test: comparisons            PASS (diff = 0)
Spike golden test: memory_widths          PASS (diff = 0)
Spike golden test: loop_sum               PASS (diff = 0)
Spike golden test: fibonacci              PASS (diff = 0)
Spike golden test: software_multiply      PASS (diff = 0)
Spike golden test: function_call          PASS (diff = 0)
Spike golden test: hazard_chain           PASS (diff = 0)
Passed 14 Spike golden test(s).
```

`diff = 0` means both models produced byte-for-byte identical canonical state:

- Final values of all 32 integer registers
- Normalized final PC
- Halt status
- Every declared memory comparison range

Generated files are stored under `build/golden/<test-name>/`.

## GitHub Actions

The `Spike Golden` CI job runs independently from the C++ and assembly-memory
tests. On a fresh Ubuntu runner it:

1. Installs the RISC-V binary utilities and Spike build dependencies.
2. Restores a cached Spike installation when available.
3. Otherwise builds a pinned Spike commit for reproducibility.
4. Builds the simulator and `spike_state_adapter`.
5. Runs `make golden`.

If either model fails to run or their canonical JSON files differ, the script
returns a nonzero exit status. GitHub attaches a failed status check to that
commit or pull request. Keeping this as a separate job ensures golden testing
still runs when an unrelated unit-test job fails.

## Why Programs Start At 0x10000

The normal learning examples start at address `0x0`. Spike reserves part of
the low address space for internal devices, so it cannot provide test RAM over
that same region.

Golden programs are therefore linked at `0x10000` using
`linker/rv32i_spike.ld`. The simulator loads the matching raw binary at the
same address with `--load-address=0x10000`. Both models consequently execute
the exact same instruction encodings with the same pointer values.

Golden programs use labels and PC-relative address generation instead of
hardcoded data addresses, which keeps them relocatable.

## Shared Halt Convention

Each golden program exports a `golden_halt` label containing `ebreak`.

The harness runs Spike until its PC reaches `golden_halt`, then reads state
before Spike treats `ebreak` as a trap. The pipelined simulator executes the
same `ebreak` as its halt convention. `spike_state_adapter` normalizes Spike's
PC to `golden_halt + 4` and marks it halted so both state files use the same
contract.

## Memory Selection

Spike does not report which addresses a program wrote. Each test therefore
declares the memory symbols and lengths that matter in
`tests/golden/cases.tsv`:

```text
store_word|tests/golden/programs/store_word.s|golden_halt|result:4|1000
```

The fields are:

```text
name | assembly source | halt symbol | memory symbol:length | max cycles
```

The manifest selects where to compare, but it does not state what values should
be there. Spike executes the program to produce the reference values, and the
harness checks the simulator's values against them. This avoids manually
calculating and hardcoding an expected memory result for each program.

Memory lengths must currently be positive multiples of four because Spike's
debug interface returns one RV32 word per memory command.

## Files

```text
tests/golden/programs/
    Relocatable bare-metal assembly fixtures.

tests/golden/cases.tsv
    Test names, halt symbols, memory regions, and cycle limits.

scripts/run_spike_golden.sh
    Builds each fixture, runs both models, and compares their JSON files.

tools/spike_state_adapter.cpp
    Converts Spike debug output into rv32i-architectural-state-v1 JSON.

linker/rv32i_spike.ld
    Links golden programs into RAM beginning at 0x10000.
```

## Add A Golden Test

1. Add a relocatable `.s` program under `tests/golden/programs/`.
2. Export `_start`, `golden_halt`, and each memory result symbol with `.globl`.
3. End at `golden_halt` with `ebreak`.
4. Add one line to `tests/golden/cases.tsv`.
5. Run `make golden` and inspect both JSON files if the diff is nonzero.

The suite currently contains 14 programs. The focused cases exercise
arithmetic, bitwise operations, shifts, comparisons, memory access widths,
loops, Fibonacci generation, software multiplication, function calls,
dependency hazards, and control flow.
