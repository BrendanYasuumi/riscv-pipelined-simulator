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
Passed 3 Spike golden test(s).
```

`diff = 0` means both models produced byte-for-byte identical canonical state:

- Final values of all 32 integer registers
- Normalized final PC
- Halt status
- Every declared memory comparison range

Generated files are stored under `build/golden/<test-name>/`.

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
