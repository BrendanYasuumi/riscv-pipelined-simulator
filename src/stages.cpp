#include "stages.hpp"

#include "instruction.hpp"

#include <cstdint>

namespace rv32i {

namespace {

int32_t sign_extend_value(uint32_t value, uint8_t bit_count) {
    const uint32_t sign_bit = uint32_t{1} << (bit_count - 1);
    const uint32_t mask = (uint32_t{1} << bit_count) - 1;
    value &= mask;

    if ((value & sign_bit) == 0) {
        return static_cast<int32_t>(value);
    }

    return static_cast<int32_t>(value | ~mask);
}

uint32_t select_writeback_value(const MEMWBRegister& mem_wb) {
    switch (mem_wb.control.writeback_source) {
        case WritebackSource::ALU:
            return mem_wb.alu_result;
        case WritebackSource::Memory:
            return mem_wb.memory_data;
        case WritebackSource::PCPlus4:
            return mem_wb.pc_plus_4;
        case WritebackSource::Immediate:
            return mem_wb.immediate;
        case WritebackSource::None:
            return 0;
    }

    return 0;
}

uint32_t execute_alu(ALUOp op, uint32_t lhs, uint32_t rhs) {
    const uint32_t shamt = rhs & 0x1Fu;

    switch (op) {
        case ALUOp::Add:
            return lhs + rhs;
        case ALUOp::Sub:
            return lhs - rhs;
        case ALUOp::And:
            return lhs & rhs;
        case ALUOp::Or:
            return lhs | rhs;
        case ALUOp::Xor:
            return lhs ^ rhs;
        case ALUOp::Sll:
            return lhs << shamt;
        case ALUOp::Srl:
            return lhs >> shamt;
        case ALUOp::Sra:
            return static_cast<uint32_t>(static_cast<int32_t>(lhs) >> shamt);
        case ALUOp::Slt:
            return static_cast<int32_t>(lhs) < static_cast<int32_t>(rhs) ? 1u
                                                                         : 0u;
        case ALUOp::Sltu:
            return lhs < rhs ? 1u : 0u;
        case ALUOp::None:
            return 0;
    }

    return 0;
}

bool evaluate_branch(BranchCondition condition, uint32_t lhs, uint32_t rhs) {
    switch (condition) {
        case BranchCondition::Equal:
            return lhs == rhs;
        case BranchCondition::NotEqual:
            return lhs != rhs;
        case BranchCondition::LessThan:
            return static_cast<int32_t>(lhs) < static_cast<int32_t>(rhs);
        case BranchCondition::GreaterOrEqual:
            return static_cast<int32_t>(lhs) >= static_cast<int32_t>(rhs);
        case BranchCondition::LessThanUnsigned:
            return lhs < rhs;
        case BranchCondition::GreaterOrEqualUnsigned:
            return lhs >= rhs;
        case BranchCondition::None:
            return false;
    }

    return false;
}

uint32_t read_memory(CPU& cpu, uint32_t address, const ControlSignals& control) {
    switch (control.memory_width) {
        case MemoryAccessWidth::Byte: {
            const uint32_t value = cpu.read_u8(address);
            if (control.mem_unsigned) {
                return value;
            }
            return static_cast<uint32_t>(sign_extend_value(value, 8));
        }
        case MemoryAccessWidth::HalfWord: {
            const uint32_t value = cpu.read_u16(address);
            if (control.mem_unsigned) {
                return value;
            }
            return static_cast<uint32_t>(sign_extend_value(value, 16));
        }
        case MemoryAccessWidth::Word:
            return cpu.read_u32(address);
        case MemoryAccessWidth::None:
            return 0;
    }

    return 0;
}

void write_memory(CPU& cpu,
                  uint32_t address,
                  uint32_t value,
                  const ControlSignals& control) {
    switch (control.memory_width) {
        case MemoryAccessWidth::Byte:
            cpu.write_u8(address, static_cast<uint8_t>(value & 0xFFu));
            break;
        case MemoryAccessWidth::HalfWord:
            cpu.write_u16(address, static_cast<uint16_t>(value & 0xFFFFu));
            break;
        case MemoryAccessWidth::Word:
            cpu.write_u32(address, value);
            break;
        case MemoryAccessWidth::None:
            break;
    }
}

}  // namespace

void stage_WB(CPU& cpu, PipelineRegisters& pipeline) {
    const MEMWBRegister& mem_wb = pipeline.mem_wb;
    if (!mem_wb.valid) {
        return;
    }

    if (mem_wb.control.reg_write) {
        cpu.write_reg(mem_wb.rd, select_writeback_value(mem_wb));
    }

    ++cpu.mutable_stats().instruction_count;
}

void stage_MEM(CPU& cpu, PipelineRegisters& pipeline) {
    const EXMEMRegister& ex_mem = pipeline.ex_mem;
    if (!ex_mem.valid) {
        pipeline.next_mem_wb.clear();
        return;
    }

    MEMWBRegister next{};
    next.valid = true;
    next.pc = ex_mem.pc;
    next.instruction = ex_mem.instruction;
    next.rd = ex_mem.rd;
    next.alu_result = ex_mem.alu_result;
    next.pc_plus_4 = ex_mem.pc_plus_4;
    next.immediate = ex_mem.immediate;
    next.control = ex_mem.control;

    if (ex_mem.control.mem_read) {
        next.memory_data = read_memory(cpu, ex_mem.alu_result, ex_mem.control);
    }

    if (ex_mem.control.mem_write) {
        write_memory(cpu, ex_mem.alu_result, ex_mem.store_value, ex_mem.control);
    }

    pipeline.next_mem_wb = next;
}

void stage_EX(CPU& cpu, PipelineRegisters& pipeline, StageControl& control) {
    const IDEXRegister& id_ex = pipeline.id_ex;
    if (!id_ex.valid) {
        pipeline.next_ex_mem.clear();
        return;
    }

    const uint32_t lhs = id_ex.control.alu_src_pc ? id_ex.pc : id_ex.rs1_value;
    const uint32_t rhs = id_ex.control.alu_src_imm
                             ? static_cast<uint32_t>(id_ex.immediate)
                             : id_ex.rs2_value;

    EXMEMRegister next{};
    next.valid = true;
    next.pc = id_ex.pc;
    next.instruction = id_ex.instruction;
    next.rd = id_ex.rd;
    next.alu_result = execute_alu(id_ex.control.alu_op, lhs, rhs);
    next.store_value = id_ex.rs2_value;
    next.pc_plus_4 = id_ex.pc + 4;
    next.immediate = static_cast<uint32_t>(id_ex.immediate);
    next.control = id_ex.control;

    if (id_ex.control.branch) {
        next.branch_taken = evaluate_branch(id_ex.control.branch_condition,
                                            id_ex.rs1_value,
                                            id_ex.rs2_value);
        next.branch_target = id_ex.pc + static_cast<uint32_t>(id_ex.immediate);
    } else if (id_ex.control.jump) {
        next.branch_taken = true;
        next.branch_target = id_ex.control.alu_src_pc
                                 ? next.alu_result
                                 : (next.alu_result & ~uint32_t{1});
    }

    if (next.branch_taken) {
        cpu.set_pc(next.branch_target);
        control.flush_id_ex = true;

        if (id_ex.control.branch) {
            ++cpu.mutable_stats().branch_mispredictions;
        }
    }

    pipeline.next_ex_mem = next;
}

void stage_ID(CPU& cpu,
              PipelineRegisters& pipeline,
              const StageControl& control) {
    if (control.flush_id_ex) {
        pipeline.next_id_ex.clear();
        return;
    }

    const IFIDRegister& if_id = pipeline.if_id;
    if (!if_id.valid) {
        pipeline.next_id_ex.clear();
        return;
    }

    const DecodedInstruction decoded = decode_instruction(if_id.instruction);
    if (!is_valid_instruction(decoded)) {
        pipeline.next_id_ex.clear();
        return;
    }

    IDEXRegister next{};
    next.valid = true;
    next.pc = if_id.pc;
    next.instruction = if_id.instruction;
    next.format = decoded.format;
    next.rd = decoded.rd;
    next.rs1 = decoded.rs1;
    next.rs2 = decoded.rs2;
    next.funct3 = decoded.funct3;
    next.funct7 = decoded.funct7;
    next.immediate = decoded.immediate;
    next.rs1_value = cpu.read_reg(decoded.rs1);
    next.rs2_value = cpu.read_reg(decoded.rs2);
    next.control = decoded.control;

    pipeline.next_id_ex = next;
}

void stage_IF(CPU& cpu, PipelineRegisters& pipeline) {
    const uint32_t pc = cpu.pc();
    if (static_cast<uint64_t>(pc) + sizeof(uint32_t) > cpu.memory_size()) {
        pipeline.next_if_id.clear();
        return;
    }

    IFIDRegister next{};
    next.valid = true;
    next.pc = pc;
    next.instruction = cpu.read_u32(pc);

    cpu.advance_pc();
    pipeline.next_if_id = next;
}

void run_pipeline_cycle(CPU& cpu, PipelineRegisters& pipeline) {
    pipeline.clear_next();

    StageControl control{};
    stage_WB(cpu, pipeline);
    stage_MEM(cpu, pipeline);
    stage_EX(cpu, pipeline, control);
    stage_ID(cpu, pipeline, control);
    stage_IF(cpu, pipeline);

    pipeline.commit();
    cpu.tick();
}

}  // namespace rv32i
