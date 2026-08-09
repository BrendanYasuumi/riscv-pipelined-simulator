#include "stages.hpp"

#include "forwarding_unit.hpp"
#include "hazard_unit.hpp"
#include "instruction.hpp"

#include <algorithm>
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

void stage_MEM(CPU& cpu, PipelineRegisters& pipeline, StageControl& control) {
    const EXMEMRegister& ex_mem = pipeline.ex_mem;
    if (!ex_mem.valid) {
        pipeline.next_mem_wb.clear();
        return;
    }

    if ((ex_mem.control.mem_read || ex_mem.control.mem_write) &&
        ex_mem.memory_cycles_remaining > 1) {
        pipeline.next_ex_mem = ex_mem;
        --pipeline.next_ex_mem.memory_cycles_remaining;
        pipeline.next_mem_wb.clear();
        control.stall_pipeline = true;
        ++cpu.mutable_stats().stall_cycles;
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

    const ForwardedOperands operands =
        resolve_forwarding(id_ex, pipeline, cpu.config().enable_forwarding);

    const uint32_t lhs = id_ex.control.alu_src_pc ? id_ex.pc
                                                  : operands.rs1_value;
    const uint32_t rhs = id_ex.control.alu_src_imm
                             ? static_cast<uint32_t>(id_ex.immediate)
                             : operands.rs2_value;

    EXMEMRegister next{};
    next.valid = true;
    next.pc = id_ex.pc;
    next.instruction = id_ex.instruction;
    next.rd = id_ex.rd;
    next.alu_result = execute_alu(id_ex.control.alu_op, lhs, rhs);
    next.store_value = operands.rs2_value;
    next.pc_plus_4 = id_ex.pc + 4;
    next.immediate = static_cast<uint32_t>(id_ex.immediate);
    next.control = id_ex.control;
    next.memory_cycles_remaining =
        (id_ex.control.mem_read || id_ex.control.mem_write)
            ? std::max<uint32_t>(1, cpu.config().memory_latency_cycles)
            : 0;

    bool control_flow_resolved = false;
    bool actual_taken = false;
    uint32_t actual_target = id_ex.pc + 4;

    if (id_ex.control.branch) {
        control_flow_resolved = true;
        actual_taken = evaluate_branch(id_ex.control.branch_condition,
                                       operands.rs1_value,
                                       operands.rs2_value);
        actual_target =
            actual_taken ? id_ex.pc + static_cast<uint32_t>(id_ex.immediate)
                         : id_ex.pc + 4;
        next.branch_taken = actual_taken;
        next.branch_target = actual_target;
        cpu.update_branch_predictor(id_ex.pc, actual_taken);
    } else if (id_ex.control.jump) {
        control_flow_resolved = true;
        actual_taken = true;
        actual_target = id_ex.control.alu_src_pc ? next.alu_result
                                                 : (next.alu_result &
                                                    ~uint32_t{1});
        next.branch_taken = true;
        next.branch_target = actual_target;
    }

    const bool mispredicted =
        control_flow_resolved &&
        (id_ex.predicted_taken != actual_taken ||
         (actual_taken && id_ex.predicted_target != actual_target));

    if (mispredicted) {
        cpu.set_pc(actual_target);
        control.flush_id_ex = true;
        ++cpu.mutable_stats().branch_mispredictions;
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

    if (control.stall_fetch_decode) {
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
    next.predicted_taken = if_id.predicted_taken;
    next.predicted_target = if_id.predicted_target;
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
    pipeline.next_if_id = pipeline.if_id;

    const uint32_t pc = cpu.pc();
    if (static_cast<uint64_t>(pc) + sizeof(uint32_t) > cpu.memory_size()) {
        pipeline.next_if_id.clear();
        return;
    }

    IFIDRegister next{};
    next.valid = true;
    next.pc = pc;
    next.instruction = cpu.read_u32(pc);
    next.predicted_target = pc + 4;

    const DecodedInstruction decoded = decode_instruction(next.instruction);
    if (is_valid_instruction(decoded)) {
        if (decoded.control.branch && cpu.predict_branch(pc)) {
            next.predicted_taken = true;
            next.predicted_target = pc + static_cast<uint32_t>(decoded.immediate);
        } else if (decoded.kind == InstructionKind::Jal) {
            next.predicted_taken = true;
            next.predicted_target = pc + static_cast<uint32_t>(decoded.immediate);
        }
    }

    cpu.set_pc(next.predicted_taken ? next.predicted_target : pc + 4);
    pipeline.next_if_id = next;
}

void run_pipeline_cycle(CPU& cpu, PipelineRegisters& pipeline) {
    pipeline.clear_next();

    StageControl control{};
    stage_WB(cpu, pipeline);
    stage_MEM(cpu, pipeline, control);

    if (control.stall_pipeline) {
        pipeline.next_id_ex = pipeline.id_ex;
        pipeline.next_if_id = pipeline.if_id;
        pipeline.commit();
        cpu.tick();
        return;
    }

    stage_EX(cpu, pipeline, control);

    if (!control.flush_id_ex) {
        const HazardDecision hazard =
            detect_raw_hazard(pipeline, cpu.config().enable_forwarding);
        control.stall_fetch_decode = hazard.stall;

        if (hazard.stall) {
            ++cpu.mutable_stats().stall_cycles;
        }
    }

    stage_ID(cpu, pipeline, control);

    if (!control.stall_fetch_decode) {
        stage_IF(cpu, pipeline);
    } else {
        pipeline.next_if_id = pipeline.if_id;
    }

    pipeline.commit();
    cpu.tick();
}

}  // namespace rv32i
