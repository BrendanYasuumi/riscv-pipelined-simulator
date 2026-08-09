#include "forwarding_unit.hpp"

namespace rv32i {

namespace {

bool can_forward_from_ex_mem(const EXMEMRegister& ex_mem) {
    return ex_mem.valid && ex_mem.control.reg_write && ex_mem.rd != 0 &&
           !ex_mem.control.mem_read;
}

bool can_forward_from_mem_wb(const MEMWBRegister& mem_wb) {
    return mem_wb.valid && mem_wb.control.reg_write && mem_wb.rd != 0;
}

uint32_t ex_mem_forward_value(const EXMEMRegister& ex_mem) {
    switch (ex_mem.control.writeback_source) {
        case WritebackSource::ALU:
            return ex_mem.alu_result;
        case WritebackSource::PCPlus4:
            return ex_mem.pc_plus_4;
        case WritebackSource::Immediate:
            return ex_mem.immediate;
        case WritebackSource::Memory:
        case WritebackSource::None:
            return 0;
    }

    return 0;
}

uint32_t mem_wb_forward_value(const MEMWBRegister& mem_wb) {
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

}  // namespace

ForwardedOperands resolve_forwarding(const IDEXRegister& id_ex,
                                     const PipelineRegisters& pipeline,
                                     bool enable_forwarding) {
    ForwardedOperands operands{};
    operands.rs1_value = id_ex.rs1_value;
    operands.rs2_value = id_ex.rs2_value;

    if (!enable_forwarding) {
        return operands;
    }

    if (can_forward_from_ex_mem(pipeline.ex_mem)) {
        const uint32_t value = ex_mem_forward_value(pipeline.ex_mem);

        if (id_ex.rs1 != 0 && id_ex.rs1 == pipeline.ex_mem.rd) {
            operands.rs1_value = value;
            operands.forwarded_rs1 = true;
        }
        if (id_ex.rs2 != 0 && id_ex.rs2 == pipeline.ex_mem.rd) {
            operands.rs2_value = value;
            operands.forwarded_rs2 = true;
        }
    }

    if (can_forward_from_mem_wb(pipeline.mem_wb)) {
        const uint32_t value = mem_wb_forward_value(pipeline.mem_wb);

        if (!operands.forwarded_rs1 && id_ex.rs1 != 0 &&
            id_ex.rs1 == pipeline.mem_wb.rd) {
            operands.rs1_value = value;
            operands.forwarded_rs1 = true;
        }
        if (!operands.forwarded_rs2 && id_ex.rs2 != 0 &&
            id_ex.rs2 == pipeline.mem_wb.rd) {
            operands.rs2_value = value;
            operands.forwarded_rs2 = true;
        }
    }

    return operands;
}

}  // namespace rv32i
