# RV32I Instruction Coverage

This audit is based on the instruction kinds implemented by
`decode_instruction`, not only the smaller original project requirement.

Every implemented instruction is assembled from a real `.s` file, executed by
the pipeline, and covered by a final memory-signature check in
`scripts/asm_memory_tests.sh`.

## Coverage Matrix

| Category | Instructions | Primary assembly test |
| --- | --- | --- |
| Register arithmetic | `add`, `sub` | `instruction_coverage.s` |
| Register bitwise | `and`, `or`, `xor` | `instruction_coverage.s` |
| Register shifts | `sll`, `srl`, `sra` | `instruction_coverage.s` |
| Register comparisons | `slt`, `sltu` | `instruction_coverage.s` |
| Immediate arithmetic | `addi` | `instruction_coverage.s` |
| Immediate bitwise | `andi`, `ori`, `xori` | `instruction_coverage.s` |
| Immediate shifts | `slli`, `srli`, `srai` | `instruction_coverage.s` |
| Immediate comparisons | `slti`, `sltiu` | `instruction_coverage.s` |
| Loads | `lb`, `lbu`, `lh`, `lhu`, `lw` | `instruction_coverage.s` |
| Stores | `sb`, `sh`, `sw` | `instruction_coverage.s` |
| Conditional branches | `beq`, `bne`, `blt`, `bge`, `bltu`, `bgeu` | `control_flow_coverage.s` |
| Upper immediates | `lui`, `auipc` | `instruction_coverage.s` |
| Jumps | `jal`, `jalr` | `control_flow_coverage.s` |
| System halt | `ecall`, `ebreak` | `control_flow_coverage.s`, `instruction_coverage.s` |
| No operation | `nop` (`addi x0, x0, 0`) | `instruction_coverage.s` |

## Edge Cases Checked

- Negative 12-bit immediate sign-extension with `addi x5, x0, -16`
- Signed versus unsigned register comparisons with `slt` and `sltu`
- Signed versus unsigned immediate comparisons with `slti` and `sltiu`
- Logical versus arithmetic right shifts with `srli` and `srai`
- Byte and halfword sign-extension with `lb` and `lh`
- Byte and halfword zero-extension with `lbu` and `lhu`
- Narrow writes with `sb` and `sh`
- Taken and not-taken behavior for every conditional branch
- Signed and unsigned branch comparisons using `-1` and `1`
- Link-register values and target redirection for `jal` and `jalr`
- `x0` remaining zero when executing the `nop` encoding

Run the complete executable audit with:

```bash
make asm-test
```
