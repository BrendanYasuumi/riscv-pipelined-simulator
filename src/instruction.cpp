#include "instruction.hpp"

#include "decoder.hpp"

namespace rv32i {

namespace {

constexpr uint8_t kOpcodeLoad = 0x03;
constexpr uint8_t kOpcodeImm = 0x13;
constexpr uint8_t kOpcodeAuipc = 0x17;
constexpr uint8_t kOpcodeStore = 0x23;
constexpr uint8_t kOpcodeReg = 0x33;
constexpr uint8_t kOpcodeLui = 0x37;
constexpr uint8_t kOpcodeBranch = 0x63;
constexpr uint8_t kOpcodeJalr = 0x67;
constexpr uint8_t kOpcodeJal = 0x6F;
constexpr uint8_t kOpcodeSystem = 0x73;

DecodedInstruction base_decoded(uint32_t instruction) {
    DecodedInstruction decoded{};
    decoded.raw = instruction;
    decoded.opcode = extract_opcode(instruction);
    decoded.rd = extract_rd(instruction);
    decoded.rs1 = extract_rs1(instruction);
    decoded.rs2 = extract_rs2(instruction);
    decoded.funct3 = extract_funct3(instruction);
    decoded.funct7 = extract_funct7(instruction);
    return decoded;
}

ControlSignals alu_reg_control(ALUOp op) {
    ControlSignals control{};
    control.reg_write = true;
    control.alu_op = op;
    control.writeback_source = WritebackSource::ALU;
    return control;
}

ControlSignals alu_imm_control(ALUOp op) {
    ControlSignals control = alu_reg_control(op);
    control.alu_src_imm = true;
    return control;
}

void set_memory_width(ControlSignals& control, uint8_t funct3) {
    switch (funct3) {
        case 0x0:
        case 0x4:
            control.memory_width = MemoryAccessWidth::Byte;
            break;
        case 0x1:
        case 0x5:
            control.memory_width = MemoryAccessWidth::HalfWord;
            break;
        case 0x2:
            control.memory_width = MemoryAccessWidth::Word;
            break;
        default:
            control.memory_width = MemoryAccessWidth::None;
            break;
    }

    control.mem_unsigned = (funct3 == 0x4 || funct3 == 0x5);
}

}  // namespace

DecodedInstruction decode_instruction(uint32_t instruction) {
    DecodedInstruction decoded = base_decoded(instruction);

    switch (decoded.opcode) {
        case kOpcodeReg: {
            const RTypeFields fields = decode_r_type(instruction);
            decoded.format = InstructionFormat::R;

            if (fields.funct3 == 0x0 && fields.funct7 == 0x00) {
                decoded.kind = InstructionKind::Add;
                decoded.control = alu_reg_control(ALUOp::Add);
            } else if (fields.funct3 == 0x0 && fields.funct7 == 0x20) {
                decoded.kind = InstructionKind::Sub;
                decoded.control = alu_reg_control(ALUOp::Sub);
            } else if (fields.funct3 == 0x1 && fields.funct7 == 0x00) {
                decoded.kind = InstructionKind::Sll;
                decoded.control = alu_reg_control(ALUOp::Sll);
            } else if (fields.funct3 == 0x2 && fields.funct7 == 0x00) {
                decoded.kind = InstructionKind::Slt;
                decoded.control = alu_reg_control(ALUOp::Slt);
            } else if (fields.funct3 == 0x3 && fields.funct7 == 0x00) {
                decoded.kind = InstructionKind::Sltu;
                decoded.control = alu_reg_control(ALUOp::Sltu);
            } else if (fields.funct3 == 0x4 && fields.funct7 == 0x00) {
                decoded.kind = InstructionKind::Xor;
                decoded.control = alu_reg_control(ALUOp::Xor);
            } else if (fields.funct3 == 0x5 && fields.funct7 == 0x00) {
                decoded.kind = InstructionKind::Srl;
                decoded.control = alu_reg_control(ALUOp::Srl);
            } else if (fields.funct3 == 0x5 && fields.funct7 == 0x20) {
                decoded.kind = InstructionKind::Sra;
                decoded.control = alu_reg_control(ALUOp::Sra);
            } else if (fields.funct3 == 0x6 && fields.funct7 == 0x00) {
                decoded.kind = InstructionKind::Or;
                decoded.control = alu_reg_control(ALUOp::Or);
            } else if (fields.funct3 == 0x7 && fields.funct7 == 0x00) {
                decoded.kind = InstructionKind::And;
                decoded.control = alu_reg_control(ALUOp::And);
            }
            break;
        }

        case kOpcodeImm: {
            const ITypeFields fields = decode_i_type(instruction);
            decoded.format = InstructionFormat::I;
            decoded.immediate = fields.imm;

            if (fields.funct3 == 0x0) {
                decoded.kind = InstructionKind::Addi;
                decoded.control = alu_imm_control(ALUOp::Add);
            } else if (fields.funct3 == 0x2) {
                decoded.kind = InstructionKind::Slti;
                decoded.control = alu_imm_control(ALUOp::Slt);
            } else if (fields.funct3 == 0x3) {
                decoded.kind = InstructionKind::Sltiu;
                decoded.control = alu_imm_control(ALUOp::Sltu);
            } else if (fields.funct3 == 0x4) {
                decoded.kind = InstructionKind::Xori;
                decoded.control = alu_imm_control(ALUOp::Xor);
            } else if (fields.funct3 == 0x6) {
                decoded.kind = InstructionKind::Ori;
                decoded.control = alu_imm_control(ALUOp::Or);
            } else if (fields.funct3 == 0x7) {
                decoded.kind = InstructionKind::Andi;
                decoded.control = alu_imm_control(ALUOp::And);
            } else if (fields.funct3 == 0x1 && decoded.funct7 == 0x00) {
                decoded.kind = InstructionKind::Slli;
                decoded.control = alu_imm_control(ALUOp::Sll);
                decoded.immediate &= 0x1F;
            } else if (fields.funct3 == 0x5 && decoded.funct7 == 0x00) {
                decoded.kind = InstructionKind::Srli;
                decoded.control = alu_imm_control(ALUOp::Srl);
                decoded.immediate &= 0x1F;
            } else if (fields.funct3 == 0x5 && decoded.funct7 == 0x20) {
                decoded.kind = InstructionKind::Srai;
                decoded.control = alu_imm_control(ALUOp::Sra);
                decoded.immediate &= 0x1F;
            }
            break;
        }

        case kOpcodeLoad: {
            const ITypeFields fields = decode_i_type(instruction);
            decoded.format = InstructionFormat::I;
            decoded.immediate = fields.imm;

            ControlSignals control{};
            control.reg_write = true;
            control.mem_read = true;
            control.alu_src_imm = true;
            control.alu_op = ALUOp::Add;
            control.writeback_source = WritebackSource::Memory;
            set_memory_width(control, fields.funct3);

            if (fields.funct3 == 0x0) {
                decoded.kind = InstructionKind::Lb;
            } else if (fields.funct3 == 0x1) {
                decoded.kind = InstructionKind::Lh;
            } else if (fields.funct3 == 0x2) {
                decoded.kind = InstructionKind::Lw;
            } else if (fields.funct3 == 0x4) {
                decoded.kind = InstructionKind::Lbu;
            } else if (fields.funct3 == 0x5) {
                decoded.kind = InstructionKind::Lhu;
            }

            if (decoded.kind != InstructionKind::Invalid) {
                decoded.control = control;
            }
            break;
        }

        case kOpcodeStore: {
            const STypeFields fields = decode_s_type(instruction);
            decoded.format = InstructionFormat::S;
            decoded.immediate = fields.imm;

            ControlSignals control{};
            control.mem_write = true;
            control.alu_src_imm = true;
            control.alu_op = ALUOp::Add;
            set_memory_width(control, fields.funct3);
            control.mem_unsigned = false;

            if (fields.funct3 == 0x0) {
                decoded.kind = InstructionKind::Sb;
            } else if (fields.funct3 == 0x1) {
                decoded.kind = InstructionKind::Sh;
            } else if (fields.funct3 == 0x2) {
                decoded.kind = InstructionKind::Sw;
            }

            if (decoded.kind != InstructionKind::Invalid) {
                decoded.control = control;
            }
            break;
        }

        case kOpcodeBranch: {
            const BTypeFields fields = decode_b_type(instruction);
            decoded.format = InstructionFormat::B;
            decoded.immediate = fields.imm;

            ControlSignals control{};
            control.branch = true;
            control.alu_op = ALUOp::Sub;

            if (fields.funct3 == 0x0) {
                decoded.kind = InstructionKind::Beq;
                control.branch_condition = BranchCondition::Equal;
            } else if (fields.funct3 == 0x1) {
                decoded.kind = InstructionKind::Bne;
                control.branch_condition = BranchCondition::NotEqual;
            } else if (fields.funct3 == 0x4) {
                decoded.kind = InstructionKind::Blt;
                control.branch_condition = BranchCondition::LessThan;
            } else if (fields.funct3 == 0x5) {
                decoded.kind = InstructionKind::Bge;
                control.branch_condition = BranchCondition::GreaterOrEqual;
            } else if (fields.funct3 == 0x6) {
                decoded.kind = InstructionKind::Bltu;
                control.branch_condition = BranchCondition::LessThanUnsigned;
            } else if (fields.funct3 == 0x7) {
                decoded.kind = InstructionKind::Bgeu;
                control.branch_condition =
                    BranchCondition::GreaterOrEqualUnsigned;
            }

            if (decoded.kind != InstructionKind::Invalid) {
                decoded.control = control;
            }
            break;
        }

        case kOpcodeLui: {
            const UTypeFields fields = decode_u_type(instruction);
            decoded.format = InstructionFormat::U;
            decoded.immediate = static_cast<int32_t>(fields.imm);
            decoded.kind = InstructionKind::Lui;
            decoded.control.reg_write = true;
            decoded.control.writeback_source = WritebackSource::Immediate;
            break;
        }

        case kOpcodeAuipc: {
            const UTypeFields fields = decode_u_type(instruction);
            decoded.format = InstructionFormat::U;
            decoded.immediate = static_cast<int32_t>(fields.imm);
            decoded.kind = InstructionKind::Auipc;
            decoded.control.reg_write = true;
            decoded.control.alu_src_pc = true;
            decoded.control.alu_src_imm = true;
            decoded.control.alu_op = ALUOp::Add;
            decoded.control.writeback_source = WritebackSource::ALU;
            break;
        }

        case kOpcodeJal: {
            const JTypeFields fields = decode_j_type(instruction);
            decoded.format = InstructionFormat::J;
            decoded.immediate = fields.imm;
            decoded.kind = InstructionKind::Jal;
            decoded.control.reg_write = true;
            decoded.control.jump = true;
            decoded.control.alu_src_pc = true;
            decoded.control.alu_src_imm = true;
            decoded.control.alu_op = ALUOp::Add;
            decoded.control.writeback_source = WritebackSource::PCPlus4;
            break;
        }

        case kOpcodeJalr: {
            const ITypeFields fields = decode_i_type(instruction);
            decoded.format = InstructionFormat::I;
            decoded.immediate = fields.imm;

            if (fields.funct3 == 0x0) {
                decoded.kind = InstructionKind::Jalr;
                decoded.control.reg_write = true;
                decoded.control.jump = true;
                decoded.control.alu_src_imm = true;
                decoded.control.alu_op = ALUOp::Add;
                decoded.control.writeback_source = WritebackSource::PCPlus4;
            }
            break;
        }

        case kOpcodeSystem: {
            const ITypeFields fields = decode_i_type(instruction);
            decoded.format = InstructionFormat::I;
            decoded.immediate = fields.imm;

            if (fields.funct3 == 0x0 && fields.imm == 0) {
                decoded.kind = InstructionKind::Ecall;
            } else if (fields.funct3 == 0x0 && fields.imm == 1) {
                decoded.kind = InstructionKind::Ebreak;
            }
            break;
        }

        default:
            break;
    }

    return decoded;
}

const char* instruction_kind_name(InstructionKind kind) {
    switch (kind) {
        case InstructionKind::Add:
            return "add";
        case InstructionKind::Sub:
            return "sub";
        case InstructionKind::Sll:
            return "sll";
        case InstructionKind::Slt:
            return "slt";
        case InstructionKind::Sltu:
            return "sltu";
        case InstructionKind::Xor:
            return "xor";
        case InstructionKind::Srl:
            return "srl";
        case InstructionKind::Sra:
            return "sra";
        case InstructionKind::Or:
            return "or";
        case InstructionKind::And:
            return "and";
        case InstructionKind::Addi:
            return "addi";
        case InstructionKind::Slti:
            return "slti";
        case InstructionKind::Sltiu:
            return "sltiu";
        case InstructionKind::Xori:
            return "xori";
        case InstructionKind::Ori:
            return "ori";
        case InstructionKind::Andi:
            return "andi";
        case InstructionKind::Slli:
            return "slli";
        case InstructionKind::Srli:
            return "srli";
        case InstructionKind::Srai:
            return "srai";
        case InstructionKind::Lb:
            return "lb";
        case InstructionKind::Lh:
            return "lh";
        case InstructionKind::Lw:
            return "lw";
        case InstructionKind::Lbu:
            return "lbu";
        case InstructionKind::Lhu:
            return "lhu";
        case InstructionKind::Sb:
            return "sb";
        case InstructionKind::Sh:
            return "sh";
        case InstructionKind::Sw:
            return "sw";
        case InstructionKind::Beq:
            return "beq";
        case InstructionKind::Bne:
            return "bne";
        case InstructionKind::Blt:
            return "blt";
        case InstructionKind::Bge:
            return "bge";
        case InstructionKind::Bltu:
            return "bltu";
        case InstructionKind::Bgeu:
            return "bgeu";
        case InstructionKind::Lui:
            return "lui";
        case InstructionKind::Auipc:
            return "auipc";
        case InstructionKind::Jal:
            return "jal";
        case InstructionKind::Jalr:
            return "jalr";
        case InstructionKind::Ecall:
            return "ecall";
        case InstructionKind::Ebreak:
            return "ebreak";
        case InstructionKind::Invalid:
            return "invalid";
    }

    return "invalid";
}

bool is_valid_instruction(const DecodedInstruction& instruction) {
    return instruction.kind != InstructionKind::Invalid;
}

}  // namespace rv32i
