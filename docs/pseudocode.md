# Simulator Pseudocode

This document mirrors the C++ implementation at an architectural level. It is
meant to be reviewed before each implementation step so the simulator remains
cycle-accurate rather than becoming a single-cycle interpreter.

## CPU State

```text
CPU:
    config
    regs[32] = all zeros
    pc = 0
    memory = byte-addressable RAM
    stats:
        clock_cycles
        instruction_count
        stall_cycles
        branch_mispredictions
```

## Register File

```text
read_reg(index):
    require 0 <= index < 32

    if index == 0:
        return 0

    return regs[index]

write_reg(index, value):
    require 0 <= index < 32

    if index == 0:
        ignore write
        return

    regs[index] = value
```

Hardware rationale: RV32I register `x0` is not an ordinary writable storage
cell. It behaves like a constant-zero source, so writes are discarded and reads
always return zero.

## Little-Endian Memory

```text
read_u32(address):
    require address + 4 <= memory.size

    return memory[address + 0]
         | memory[address + 1] << 8
         | memory[address + 2] << 16
         | memory[address + 3] << 24

write_u32(address, value):
    require address + 4 <= memory.size

    memory[address + 0] = value[7:0]
    memory[address + 1] = value[15:8]
    memory[address + 2] = value[23:16]
    memory[address + 3] = value[31:24]
```

Hardware rationale: memory is byte-addressable, while RV32I instructions are
32-bit words. Fetch reconstructs the instruction from four consecutive bytes in
little-endian order.

## Instruction Decoder

```text
extract_bits(value, high_bit, low_bit):
    width = high_bit - low_bit + 1
    mask = (1 << width) - 1
    return (value >> low_bit) & mask
```

```text
decode_r_type(instruction):
    opcode = instruction[6:0]
    rd     = instruction[11:7]
    funct3 = instruction[14:12]
    rs1    = instruction[19:15]
    rs2    = instruction[24:20]
    funct7 = instruction[31:25]
```

```text
decode_i_type(instruction):
    opcode = instruction[6:0]
    rd     = instruction[11:7]
    funct3 = instruction[14:12]
    rs1    = instruction[19:15]
    imm    = sign_extend(instruction[31:20], 12)
```

```text
decode_s_type(instruction):
    opcode = instruction[6:0]
    funct3 = instruction[14:12]
    rs1    = instruction[19:15]
    rs2    = instruction[24:20]
    imm    = sign_extend(instruction[31:25] concat instruction[11:7], 12)
```

```text
decode_b_type(instruction):
    opcode = instruction[6:0]
    funct3 = instruction[14:12]
    rs1    = instruction[19:15]
    rs2    = instruction[24:20]
    imm    = sign_extend({
                 instruction[31],
                 instruction[7],
                 instruction[30:25],
                 instruction[11:8],
                 0
             }, 13)
```

```text
decode_u_type(instruction):
    opcode = instruction[6:0]
    rd     = instruction[11:7]
    imm    = instruction[31:12] followed by 12 zero bits
```

```text
decode_j_type(instruction):
    opcode = instruction[6:0]
    rd     = instruction[11:7]
    imm    = sign_extend({
                 instruction[31],
                 instruction[19:12],
                 instruction[20],
                 instruction[30:21],
                 0
             }, 21)
```

Hardware rationale: RISC-V keeps common fields like `opcode`, `rd`, `rs1`,
and `rs2` in stable bit positions so the hardware decoder can be simple.
Immediate fields are split differently by format because each instruction type
spends its 32 bits on different operands.

## Pipeline Registers

```text
ControlSignals:
    reg_write
    mem_read
    mem_write
    branch
    jump
    alu_src_imm
    alu_op
    writeback_source
```

Control signals are the decoded "intent" of an instruction. For example, an
`add` instruction enables register writeback but does not read or write memory.
A `sw` instruction writes memory but does not write a destination register.

```text
IF/ID latch:
    valid
    pc
    instruction
```

The fetch stage writes this latch. The decode stage reads it on the next cycle.

```text
ID/EX latch:
    valid
    pc
    instruction
    format
    rd
    rs1
    rs2
    funct3
    funct7
    immediate
    rs1_value
    rs2_value
    control
```

The decode stage writes this latch after reading the register file and
generating control signals. The execute stage reads it on the next cycle.

```text
EX/MEM latch:
    valid
    pc
    instruction
    rd
    alu_result
    store_value
    branch_target
    branch_taken
    memory_cycles_remaining
    control
```

The execute stage writes this latch after running the ALU or computing a branch
target. The memory stage reads it on the next cycle.

```text
MEM/WB latch:
    valid
    pc
    instruction
    rd
    alu_result
    memory_data
    pc_plus_4
    immediate
    control
```

The memory stage writes this latch after loads or stores finish. The writeback
stage reads it on the next cycle.

```text
commit_pipeline_registers():
    IF/ID  = next IF/ID
    ID/EX  = next ID/EX
    EX/MEM = next EX/MEM
    MEM/WB = next MEM/WB

clear_next_pipeline_registers():
    next IF/ID  = empty bubble
    next ID/EX  = empty bubble
    next EX/MEM = empty bubble
    next MEM/WB = empty bubble
```

Hardware rationale: a latch separates two stages. During one cycle, each stage
reads the current latch and computes the next latch. At the clock edge, all
next latches become current together. A `valid = false` latch is a bubble, which
means no real instruction occupies that stage.

## Cycle Accounting

```text
tick():
    stats.clock_cycles += 1

cpi():
    if stats.instruction_count == 0:
        return 0
    return stats.clock_cycles / stats.instruction_count

ipc():
    if stats.clock_cycles == 0:
        return 0
    return stats.instruction_count / stats.clock_cycles
```

Hardware rationale: the global clock is advanced explicitly by the simulator
loop. Later, each call to `tick()` will represent one rising clock edge where
pipeline latches capture their next-state values.

## Future Five-Stage Pipeline Loop

```text
while simulation_running:
    stage_WB(cpu, mem_wb)
    stage_MEM(cpu, ex_mem, next_mem_wb)
    stage_EX(cpu, id_ex, next_ex_mem)
    stage_ID(cpu, if_id, next_id_ex)
    stage_IF(cpu, next_if_id)

    commit next pipeline registers
    cpu.tick()
```

Hardware rationale: stages are evaluated in reverse order so each stage reads
the previous cycle's latch values and writes only to next-cycle latch values.
That models simultaneous hardware behavior instead of letting one instruction
teleport through multiple stages in a single cycle.
