#include "hazard_unit.hpp"

namespace rv32i {

namespace {

bool writes_register(bool valid, const ControlSignals& control, uint8_t rd) {
    return valid && control.reg_write && rd != 0;
}

bool depends_on(const SourceRegisters& sources, uint8_t rd) {
    return (sources.uses_rs1 && sources.rs1 == rd) ||
           (sources.uses_rs2 && sources.rs2 == rd);
}

}  // namespace

SourceRegisters get_source_registers(const DecodedInstruction& instruction) {
    SourceRegisters sources{};
    sources.rs1 = instruction.rs1;
    sources.rs2 = instruction.rs2;

    switch (instruction.kind) {
        case InstructionKind::Add:
        case InstructionKind::Sub:
        case InstructionKind::Sll:
        case InstructionKind::Slt:
        case InstructionKind::Sltu:
        case InstructionKind::Xor:
        case InstructionKind::Srl:
        case InstructionKind::Sra:
        case InstructionKind::Or:
        case InstructionKind::And:
        case InstructionKind::Sb:
        case InstructionKind::Sh:
        case InstructionKind::Sw:
        case InstructionKind::Beq:
        case InstructionKind::Bne:
        case InstructionKind::Blt:
        case InstructionKind::Bge:
        case InstructionKind::Bltu:
        case InstructionKind::Bgeu:
            sources.uses_rs1 = true;
            sources.uses_rs2 = true;
            break;

        case InstructionKind::Addi:
        case InstructionKind::Slti:
        case InstructionKind::Sltiu:
        case InstructionKind::Xori:
        case InstructionKind::Ori:
        case InstructionKind::Andi:
        case InstructionKind::Slli:
        case InstructionKind::Srli:
        case InstructionKind::Srai:
        case InstructionKind::Lb:
        case InstructionKind::Lh:
        case InstructionKind::Lw:
        case InstructionKind::Lbu:
        case InstructionKind::Lhu:
        case InstructionKind::Jalr:
            sources.uses_rs1 = true;
            break;

        case InstructionKind::Lui:
        case InstructionKind::Auipc:
        case InstructionKind::Jal:
        case InstructionKind::Ecall:
        case InstructionKind::Ebreak:
        case InstructionKind::Invalid:
            break;
    }

    if (sources.rs1 == 0) {
        sources.uses_rs1 = false;
    }
    if (sources.rs2 == 0) {
        sources.uses_rs2 = false;
    }

    return sources;
}

HazardDecision detect_raw_hazard(const PipelineRegisters& pipeline) {
    HazardDecision decision{};

    if (!pipeline.if_id.valid) {
        return decision;
    }

    const DecodedInstruction decoded =
        decode_instruction(pipeline.if_id.instruction);
    if (!is_valid_instruction(decoded)) {
        return decision;
    }

    const SourceRegisters sources = get_source_registers(decoded);

    if (writes_register(pipeline.id_ex.valid,
                        pipeline.id_ex.control,
                        pipeline.id_ex.rd) &&
        depends_on(sources, pipeline.id_ex.rd)) {
        decision.stall = true;
        return decision;
    }

    if (writes_register(pipeline.ex_mem.valid,
                        pipeline.ex_mem.control,
                        pipeline.ex_mem.rd) &&
        depends_on(sources, pipeline.ex_mem.rd)) {
        decision.stall = true;
        return decision;
    }

    return decision;
}

}  // namespace rv32i
