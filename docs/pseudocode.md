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

## Assembly Memory Regression

```text
make asm-test:
    build the simulator if its sources changed

    for each case declared in scripts/asm_memory_tests.sh:
        assemble and link the case
        convert its ELF into a flat binary
        run the binary with its maximum cycle limit
        check every expected address:value pair

        if every expected word matches:
            print PASS
        otherwise:
            print FAIL, show the test log, and stop with a nonzero exit status

GitHub Actions:
    build-and-test job:
        check out the repository
        install RISC-V binutils
        build the simulator
        run make test
        run make asm-test

    Spike-golden job, running independently:
        check out the repository
        install RISC-V binutils and Spike build dependencies
        restore cached pinned Spike, or build it when not cached
        build the simulator and Spike state adapter
        run make golden

    mark either job failed if one of its commands returns a nonzero exit status

make asm-test-failure-demo:
    run arithmetic_smoke with an intentionally incorrect expected value
    print the expected and actual memory words
    return a nonzero exit status, matching what CI treats as a failure
    do not include this demonstration target in the normal CI workflow
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
        branch predictions
        branch mispredictions

    branch predictor:
        64 two-bit saturating counters
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
        compare actual next PC with predicted next PC
        train predictor for a conditional branch
        redirect PC and flush younger work if prediction was wrong
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
        if instruction is a conditional branch:
            consult predictor using branch PC
            choose branch target or PC + 4
            save prediction in IF/ID
        write next IF/ID latch
        PC = predicted next PC

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
predict conditional branch in IF:
    index = (branch PC >> 2) modulo 64
    state = counter[index]

    if state is Weakly Taken or Strongly Taken:
        predicted taken = true
        predicted next PC = branch target
    else:
        predicted taken = false
        predicted next PC = PC + 4

    carry predicted direction and next PC through IF/ID and ID/EX

resolve conditional branch in EX:
    evaluate condition using forwarded operands
    actual next PC = branch target if taken, otherwise PC + 4
    branch_predictions += 1

    if predicted next PC does not equal actual next PC:
        branch_mispredictions += 1
        PC = actual next PC
        flush younger instruction

    if actual outcome is taken:
        increment counter, saturating at Strongly Taken
    else:
        decrement counter, saturating at Strongly Not Taken

resolve jump in EX:
    compute direct or indirect target
    redirect PC and flush if target differs from sequential prediction
```
