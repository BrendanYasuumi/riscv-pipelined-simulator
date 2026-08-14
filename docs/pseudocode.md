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

## Instruction Classification and Control

```text
DecodedInstruction:
    kind
    format
    raw
    opcode
    rd
    rs1
    rs2
    funct3
    funct7
    immediate
    control
```

Instruction classification is the control-unit step. The earlier decoder only
extracts fields; this step decides what those fields mean.

```text
decode_instruction(raw):
    decoded.opcode = raw[6:0]
    decoded.rd     = raw[11:7]
    decoded.rs1    = raw[19:15]
    decoded.rs2    = raw[24:20]
    decoded.funct3 = raw[14:12]
    decoded.funct7 = raw[31:25]

    if opcode == R_TYPE:
        decoded.format = R

        if funct3 == 0 and funct7 == 0x00:
            decoded.kind = ADD
            control.reg_write = true
            control.alu_op = ADD
            control.writeback_source = ALU

        if funct3 == 0 and funct7 == 0x20:
            decoded.kind = SUB
            control.reg_write = true
            control.alu_op = SUB
            control.writeback_source = ALU
```

```text
    if opcode == I_TYPE_ALU:
        decoded.format = I
        decoded.immediate = sign_extend(raw[31:20], 12)

        decoded.kind = ADDI / ANDI / ORI / ...
        control.reg_write = true
        control.alu_src_imm = true
        control.alu_op = operation selected by funct3/funct7
        control.writeback_source = ALU
```

```text
    if opcode == LOAD:
        decoded.format = I
        decoded.immediate = sign_extend(raw[31:20], 12)

        decoded.kind = LB / LH / LW / LBU / LHU
        control.reg_write = true
        control.mem_read = true
        control.alu_src_imm = true
        control.alu_op = ADD
        control.writeback_source = MEMORY
        control.memory_width = byte / halfword / word
        control.mem_unsigned = true for LBU and LHU
```

```text
    if opcode == STORE:
        decoded.format = S
        decoded.immediate = sign_extend(raw[31:25] concat raw[11:7], 12)

        decoded.kind = SB / SH / SW
        control.mem_write = true
        control.alu_src_imm = true
        control.alu_op = ADD
        control.memory_width = byte / halfword / word
```

```text
    if opcode == BRANCH:
        decoded.format = B
        decoded.immediate = sign_extend(reassembled branch offset, 13)

        decoded.kind = BEQ / BNE / BLT / BGE / BLTU / BGEU
        control.branch = true
        control.branch_condition = selected comparison
```

```text
    if opcode == LUI:
        decoded.format = U
        decoded.kind = LUI
        decoded.immediate = raw[31:12] followed by 12 zero bits
        control.reg_write = true
        control.writeback_source = IMMEDIATE

    if opcode == AUIPC:
        decoded.format = U
        decoded.kind = AUIPC
        decoded.immediate = raw[31:12] followed by 12 zero bits
        control.reg_write = true
        control.alu_src_pc = true
        control.alu_src_imm = true
        control.alu_op = ADD
        control.writeback_source = ALU
```

```text
    if opcode == JAL or JALR:
        decoded.kind = JAL / JALR
        control.reg_write = true
        control.jump = true
        control.writeback_source = PC_PLUS_4
```

Hardware rationale: the decode stage is where instruction bits become control
signals. Later stages should not need to rediscover whether an instruction is a
load, store, branch, ALU operation, or jump; they should simply follow the
latched control signals.

## Five-Stage Pipeline Skeleton

```text
run_pipeline_cycle(cpu, pipeline):
    pipeline.clear_next()

    control = empty stage control

    stage_WB(cpu, pipeline)
    stage_MEM(cpu, pipeline)
    stage_EX(cpu, pipeline, control)
    stage_ID(cpu, pipeline, control)
    stage_IF(cpu, pipeline)

    pipeline.commit()
    cpu.tick()
```

Stages run in reverse order so every stage reads the current pipeline latch and
writes the next pipeline latch. The `commit()` call models the clock edge.

```text
stage_IF:
    if PC is outside memory:
        next IF/ID = bubble
        return

    instruction = memory[PC]
    next IF/ID.valid = true
    next IF/ID.pc = PC
    next IF/ID.instruction = instruction
    PC = PC + 4
```

Fetch reads the next raw instruction word. For now, fetch assumes sequential
execution unless EX redirects the PC for a taken branch or jump.

```text
stage_ID:
    if EX requested a flush:
        next ID/EX = bubble
        return

    if current IF/ID is not valid:
        next ID/EX = bubble
        return

    decoded = decode_instruction(IF/ID.instruction)

    if decoded is invalid:
        next ID/EX = bubble
        return

    next ID/EX.valid = true
    next ID/EX.pc = IF/ID.pc
    next ID/EX decoded fields = decoded fields
    next ID/EX.rs1_value = register_file[rs1]
    next ID/EX.rs2_value = register_file[rs2]
    next ID/EX.control = decoded.control
```

Decode turns instruction bits into control signals and reads the register file.

```text
stage_EX:
    if current ID/EX is not valid:
        next EX/MEM = bubble
        return

    forwarded = resolve_forwarding(ID/EX)

    lhs = PC if control.alu_src_pc else forwarded.rs1_value
    rhs = immediate if control.alu_src_imm else forwarded.rs2_value
    alu_result = execute ALU operation

    next EX/MEM.valid = true
    next EX/MEM.alu_result = alu_result
    next EX/MEM.store_value = forwarded.rs2_value
    next EX/MEM.pc_plus_4 = PC + 4
    next EX/MEM.immediate = immediate
    next EX/MEM.control = control

    if branch is taken or jump redirects:
        PC = target
        flush younger decode work
```

Execute performs the ALU operation, branch comparison, and target-address
calculation.

```text
stage_MEM:
    if current EX/MEM is not valid:
        next MEM/WB = bubble
        return

    next MEM/WB = forwarded EX/MEM data

    if control.mem_read:
        next MEM/WB.memory_data = memory[alu_result]

    if control.mem_write:
        memory[alu_result] = store_value
```

Memory uses the ALU result as an address for loads and stores. In this first
skeleton, memory behaves as a single-cycle access.

```text
stage_WB:
    if current MEM/WB is not valid:
        return

    if control.reg_write:
        value = select ALU / memory / PC+4 / immediate
        register_file[rd] = value

    instruction_count += 1
```

Writeback commits the architectural register result and counts the instruction
as retired.

## RAW Hazard Detection

```text
get_source_registers(decoded_instruction):
    sources = empty

    if instruction is R-type ALU, store, or branch:
        sources.rs1 = decoded.rs1
        sources.rs2 = decoded.rs2

    if instruction is I-type ALU, load, or jalr:
        sources.rs1 = decoded.rs1

    if source is x0:
        ignore it because x0 is always ready

    return sources
```

```text
detect_raw_hazard(pipeline, enable_forwarding):
    if IF/ID is empty:
        return no stall

    decoded = decode_instruction(IF/ID.instruction)
    sources = get_source_registers(decoded)

    if ID/EX will write rd and rd matches a source:
        if forwarding is disabled:
            return stall
        if ID/EX is a load:
            return stall
        return no stall

    if forwarding is disabled and EX/MEM will write rd and rd matches a source:
        return stall

    return no stall
```

```text
on RAW hazard:
    next IF/ID = current IF/ID
    PC stays the same
    next ID/EX = bubble
    stats.stall_cycles += 1
```

Hardware rationale: without forwarding, a dependent instruction must wait until
the older producer reaches writeback. The bubble gives the older instruction
time to move forward while the younger instruction waits in decode.

## Forwarding

```text
resolve_forwarding(ID/EX, pipeline, enable_forwarding):
    operands.rs1_value = ID/EX.rs1_value
    operands.rs2_value = ID/EX.rs2_value

    if forwarding is disabled:
        return operands

    if EX/MEM will write rd and EX/MEM is not a load:
        if rd == ID/EX.rs1:
            operands.rs1_value = EX/MEM result
        if rd == ID/EX.rs2:
            operands.rs2_value = EX/MEM result

    if MEM/WB will write rd and the source was not already forwarded:
        if rd == ID/EX.rs1:
            operands.rs1_value = MEM/WB writeback value
        if rd == ID/EX.rs2:
            operands.rs2_value = MEM/WB writeback value

    return operands
```

```text
EX/MEM forward value:
    ALU       -> EX/MEM.alu_result
    PC_PLUS_4 -> EX/MEM.pc_plus_4
    IMMEDIATE -> EX/MEM.immediate
    MEMORY    -> cannot forward from EX/MEM

MEM/WB forward value:
    ALU       -> MEM/WB.alu_result
    MEMORY    -> MEM/WB.memory_data
    PC_PLUS_4 -> MEM/WB.pc_plus_4
    IMMEDIATE -> MEM/WB.immediate
```

Hardware rationale: forwarding is a bypass network. It sends a newly computed
value directly to the execute stage instead of waiting for the register file to
be updated. `EX/MEM` has priority over `MEM/WB` because it contains the newer
value if both stages target the same register.

## Hex Program Loader

```text
load_hex_program(path):
    open file

    for each line:
        remove anything after '#'
        trim whitespace

        if line is empty:
            continue

        remove optional '0x' prefix
        parse line as 32-bit hex word
        append word to program bytes in little-endian order
        instruction_count += 1

    if instruction_count == 0:
        error

    return program bytes and instruction count
```

Hardware rationale: the simulator memory is byte-addressable, but the easiest
human-editable program format is one 32-bit instruction word per line.

```text
load_binary_program(path):
    open file in binary mode
    read every byte into program bytes

    if byte count is zero:
        error

    if byte count is not divisible by 4:
        error

    instruction_count = byte_count / 4
    return program bytes and instruction count
```

```text
load_program(path, format):
    if format is auto:
        if path ends in ".hex":
            use hex loader
        else if path ends in ".bin":
            use binary loader
        else:
            error and ask user for --format

    if format is hex:
        return load_hex_program(path)

    if format is bin:
        return load_binary_program(path)
```

## Branch Prediction

```text
predict_branch(pc):
    if predictor is always-not-taken:
        return false

    if predictor is always-taken:
        return true

    if predictor is two-bit:
        counter = predictor_table[(pc >> 2) mod table_size]
        return counter >= 2
```

```text
update_branch_predictor(pc, taken):
    if predictor is not two-bit:
        return

    counter = predictor_table[(pc >> 2) mod table_size]

    if taken and counter < 3:
        counter += 1

    if not taken and counter > 0:
        counter -= 1
```

```text
stage_IF with prediction:
    instruction = memory[PC]
    decoded = decode_instruction(instruction)

    predicted_taken = false
    predicted_target = PC + 4

    if decoded is branch and predictor says taken:
        predicted_taken = true
        predicted_target = PC + immediate

    if decoded is JAL:
        predicted_taken = true
        predicted_target = PC + immediate

    next IF/ID.predicted_taken = predicted_taken
    next IF/ID.predicted_target = predicted_target
    PC = predicted_target if predicted_taken else PC + 4
```

```text
stage_EX branch resolution:
    actual_taken = evaluate branch condition
    actual_target = branch target if taken else PC + 4

    if predicted_taken != actual_taken:
        misprediction

    if actual_taken and predicted_target != actual_target:
        misprediction

    if misprediction:
        PC = actual_target
        flush younger decode work
        branch_mispredictions += 1

    update branch predictor with actual outcome
```

Hardware rationale: fetch must choose a next PC before the branch reaches EX.
Prediction guesses that next PC. EX later verifies the guess and flushes
wrong-path work if needed.

## Memory Latency

```text
when EX sends load/store to EX/MEM:
    memory_cycles_remaining = max(1, config.memory_latency_cycles)
```

```text
stage_MEM:
    if load/store and memory_cycles_remaining > 1:
        next EX/MEM = current EX/MEM
        next EX/MEM.memory_cycles_remaining -= 1
        next MEM/WB = bubble
        stall whole younger pipeline
        stall_cycles += 1
        return

    if load:
        read memory into MEM/WB

    if store:
        write memory
```

Hardware rationale: a slow memory operation occupies MEM for multiple cycles.
Younger instructions cannot advance past EX into MEM while the memory stage is
busy, so the simulator freezes younger latches.

## CLI Simulation Loop

```text
main:
    parse CLI options
    load built-in demo or external hex program
    create CPU with selected Config
    load program bytes into memory
    target_retired = retire-count option or loaded instruction count

    while retired instructions < target_retired:
        if clock_cycles reached max_cycles:
            stop with failure
        if trace is enabled:
            print current pipeline latches
        run_pipeline_cycle(cpu, pipeline)

    print registers
    print stats
    if dump-regs option is enabled:
        print full register dump
    if dump-memory option is enabled:
        print requested memory byte range
```

## Register and Memory Dumps

```text
print_register_dump(cpu):
    for register x0 through x31:
        value = cpu.read_reg(register)
        print register name
        print value in hexadecimal
        print value in decimal
```

```text
print_memory_dump(cpu, start_address, byte_count):
    require start_address + byte_count <= memory size

    for each 16-byte row in requested range:
        print row starting address
        for each byte in row:
            print byte as two hex digits
```

Hardware rationale: register and memory dumps are observation tools. They do
not mutate architectural state. They let you verify the final visible behavior
of a program after the pipeline timing machinery has retired instructions.

## Pipeline Trace

```text
print_pipeline_trace(cpu, pipeline):
    print cpu.stats.clock_cycles

    for each latch in IF/ID, ID/EX, EX/MEM, MEM/WB:
        if latch is invalid:
            print "bubble"
        else:
            decode latch.instruction
            print latch name, PC, raw instruction, instruction name
```

Hardware rationale: trace output is an observation tool. It does not change the
pipeline state; it simply prints the current latch contents before the next
clock cycle updates them.

```text
write_pipeline_trace_csv_header(output):
    write:
        cycle, pc, if_id, id_ex, ex_mem, mem_wb,
        retired, stalls, branch_mispredictions
```

```text
write_pipeline_trace_csv_row(cpu, pipeline):
    write current clock cycle
    write current PC
    write instruction name or bubble for each latch
    write retired instruction count
    write stall count
    write branch misprediction count
```

CSV trace rationale: text trace is optimized for humans reading the terminal.
CSV trace is optimized for external tools such as spreadsheets, plotting
scripts, or future visualizers.

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
