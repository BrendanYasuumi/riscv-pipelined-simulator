# Simulator Pseudocode

## Assembly Run Workflow

```text
run assembly program:
    input = asmFiles/program.s

    assembler:
        program.s -> program.o

    linker:
        program.o -> program.elf at address 0x0

    objcopy:
        program.elf -> program.bin

    simulator:
        load program.bin bytes into memory starting at address 0
        set PC = 0
        run pipeline cycles until halt or max cycles
        print registers
        print memory locations written by the program
```

## CPU State

```text
CPU:
    registers[32]
    pc
    memory bytes
    list of memory writes
    halted flag
    stats:
        clock cycles
        instructions retired
        stall cycles
```

```text
read register index:
    if index is x0:
        return 0
    return registers[index]

write register index, value:
    if index is x0:
        ignore write
    else:
        registers[index] = value
```

## Little-Endian Word Access

```text
read_u32(address):
    byte0 = memory[address + 0]
    byte1 = memory[address + 1]
    byte2 = memory[address + 2]
    byte3 = memory[address + 3]

    return byte0 | byte1 << 8 | byte2 << 16 | byte3 << 24
```

```text
write_u32(address, value):
    memory[address + 0] = value bits  7..0
    memory[address + 1] = value bits 15..8
    memory[address + 2] = value bits 23..16
    memory[address + 3] = value bits 31..24
    remember that bytes address through address + 3 were written
```

```text
dump_written_memory:
    if no memory writes happened:
        print "<no memory writes>"

    mark every byte touched by a memory write
    group adjacent touched bytes into ranges
    print the final byte values for each written range
```

## Machine-Readable Architectural State

```text
dump_architectural_state_json(requested_memory_ranges):
    output schema version
    output final PC
    output halted true or false

    for register x0 through x31:
        output fixed-width 32-bit hexadecimal value

    if explicit memory ranges were requested:
        select those bytes
    else:
        select every byte written during execution

    validate selected bytes are inside simulated RAM
    merge adjacent and overlapping selections
    output each range in ascending address order
```

Stable ordering and fixed-width values allow another RISC-V model to emit the
same schema for a future field-by-field comparison.

## Spike Golden Comparison

```text
for each golden test case:
    assemble one relocatable RV32I source file
    link ELF at address 0x10000
    convert ELF payload to raw binary
    resolve halt and memory symbols from ELF

    run raw binary on pipelined simulator at address 0x10000
    export simulator-state.json

    run the same ELF on Spike
    stop Spike when PC reaches golden_halt
    read all registers, PC, and declared memory words
    normalize Spike halt state and PC
    export spike-state.json through the shared serializer

    if simulator-state.json exactly equals spike-state.json:
        PASS with diff = 0
    else:
        print the differing fields and FAIL
```

## Pipeline Cycle

```text
run_pipeline_cycle:
    clear next pipeline latches

    WB:
        retire oldest instruction
        write result to register file
        mark CPU halted if instruction is halt

    MEM:
        perform load or store if needed
        write next MEM/WB latch

    EX:
        select operands
        apply forwarding if needed
        run ALU
        evaluate branch or jump
        redirect PC and flush younger work if control flow is taken
        write next EX/MEM latch

    hazard unit:
        if instruction in IF/ID depends on a load in ID/EX:
            stall fetch/decode
            insert bubble into ID/EX

    ID:
        decode instruction from IF/ID
        read source registers
        generate control signals
        write next ID/EX latch

    halt barrier:
        if a halt instruction is in ID, EX, MEM, or WB:
            stop fetching younger instructions

    IF:
        fetch instruction at PC
        write next IF/ID latch
        PC = PC + 4

    commit:
        IF/ID  = next IF/ID
        ID/EX  = next ID/EX
        EX/MEM = next EX/MEM
        MEM/WB = next MEM/WB

    clock_cycles += 1
```

## Decode

```text
decode instruction:
    extract opcode
    extract rd
    extract rs1
    extract rs2
    extract funct3
    extract funct7
    extract immediate based on instruction format

    match opcode/funct fields to instruction kind
    create control signals
```

Examples:

```text
addi:
    reg_write = true
    alu_op = add
    alu_src_imm = true
    writeback_source = ALU

sw:
    mem_write = true
    alu_op = add
    alu_src_imm = true
    memory_width = word

lw:
    reg_write = true
    mem_read = true
    alu_op = add
    alu_src_imm = true
    writeback_source = memory

beq:
    branch = true
    branch_condition = equal

jal:
    jump = true
    reg_write = true
    writeback_source = PC + 4

ecall:
    halt = true
```

## Forwarding

```text
resolve_forwarding:
    start with operand values read by ID

    if EX/MEM will write the same register needed by EX:
        use EX/MEM value

    else if MEM/WB will write the same register needed by EX:
        use MEM/WB value

    never forward to or from x0
```

## Load-Use Stall

```text
detect_load_use_hazard:
    decode instruction currently in IF/ID
    find which source registers it reads

    if ID/EX is a load
       and ID/EX.rd is not x0
       and IF/ID uses ID/EX.rd:
           stall = true
```

Hardware action:

```text
if load-use stall:
    keep PC the same
    keep IF/ID the same
    clear next ID/EX so a bubble enters EX
    stall_cycles += 1
```

## Branches And Jumps

```text
in IF:
    fetch instruction at PC
    PC = PC + 4

in EX:
    if branch condition is true:
        PC = branch target
        flush younger instruction

    if jump:
        PC = jump target
        flush younger instruction
```

This simulator does not use branch prediction. Taken control flow is handled by
redirecting once the branch or jump reaches EX.
