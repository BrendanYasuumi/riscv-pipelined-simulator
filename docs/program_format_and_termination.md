# Program Format And Termination

This simulator accepts two program formats:

- `.hex`: human-readable text, one 32-bit instruction word per line.
- `.bin`: raw bytes exactly as they should appear in simulated memory.

The CPU does not execute assembly text such as `add x3, x1, x2` directly.
Assembly must first become a 32-bit RV32I instruction encoding such as
`002081b3`.

## Recommended Program Shape

For new examples, prefer this structure:

```text
instruction
instruction
instruction
ecall
```

The final `ecall` is the program halt marker. The simulator also treats
`ebreak` as halt.

## Why Halt Is Preferred

Older examples can stop with `--retire-count=N`, which tells the simulator how
many instructions should retire before stopping.

That is useful for short branch demonstrations, but a halt instruction is
cleaner because the program itself says when it is done:

```text
main loop:
    run one cycle
    stop when CPU.halted is true
```

## NOP

RISC-V represents `nop` as:

```text
addi x0, x0, 0
```

This simulator already supports that behavior because `addi` is implemented and
writes to `x0` are ignored.

## Golden Regression Outputs

Golden regression tests compare final user-visible output:

```text
actual simulator output
expected golden output
```

The expected outputs live in:

```text
tests/golden/
```

When `diff` prints nothing, the outputs match exactly.
