#pragma once

#include <cstdint>

namespace rv32i {

enum class InstructionFormat {
    Unknown,
    R,
    I,
    S,
    B,
    U,
    J
};

enum class ALUOp {
    None,
    Add,
    Sub,
    And,
    Or,
    Xor,
    Sll,
    Srl,
    Sra,
    Slt,
    Sltu
};

enum class WritebackSource {
    None,
    ALU,
    Memory,
    PCPlus4,
    Immediate
};

enum class BranchCondition {
    None,
    Equal,
    NotEqual,
    LessThan,
    GreaterOrEqual,
    LessThanUnsigned,
    GreaterOrEqualUnsigned
};

enum class MemoryAccessWidth {
    None,
    Byte,
    HalfWord,
    Word
};

struct ControlSignals {
    bool reg_write = false;
    bool mem_read = false;
    bool mem_write = false;
    bool branch = false;
    bool jump = false;
    bool alu_src_pc = false;
    bool alu_src_imm = false;
    bool mem_unsigned = false;

    ALUOp alu_op = ALUOp::None;
    WritebackSource writeback_source = WritebackSource::None;
    BranchCondition branch_condition = BranchCondition::None;
    MemoryAccessWidth memory_width = MemoryAccessWidth::None;
};

struct IFIDRegister {
    bool valid = false;
    uint32_t pc = 0;
    uint32_t instruction = 0;

    void clear() {
        *this = IFIDRegister{};
    }
};

struct IDEXRegister {
    bool valid = false;
    uint32_t pc = 0;
    uint32_t instruction = 0;

    InstructionFormat format = InstructionFormat::Unknown;
    uint8_t rd = 0;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
    uint8_t funct3 = 0;
    uint8_t funct7 = 0;
    int32_t immediate = 0;

    uint32_t rs1_value = 0;
    uint32_t rs2_value = 0;

    ControlSignals control{};

    void clear() {
        *this = IDEXRegister{};
    }
};

struct EXMEMRegister {
    bool valid = false;
    uint32_t pc = 0;
    uint32_t instruction = 0;

    uint8_t rd = 0;
    uint32_t alu_result = 0;
    uint32_t store_value = 0;
    uint32_t pc_plus_4 = 0;
    uint32_t immediate = 0;
    uint32_t branch_target = 0;
    bool branch_taken = false;

    // Multi-cycle memory uses this to hold the MEM stage until access finishes.
    uint32_t memory_cycles_remaining = 0;

    ControlSignals control{};

    void clear() {
        *this = EXMEMRegister{};
    }
};

struct MEMWBRegister {
    bool valid = false;
    uint32_t pc = 0;
    uint32_t instruction = 0;

    uint8_t rd = 0;
    uint32_t alu_result = 0;
    uint32_t memory_data = 0;
    uint32_t pc_plus_4 = 0;
    uint32_t immediate = 0;

    ControlSignals control{};

    void clear() {
        *this = MEMWBRegister{};
    }
};

struct PipelineRegisters {
    IFIDRegister if_id{};
    IDEXRegister id_ex{};
    EXMEMRegister ex_mem{};
    MEMWBRegister mem_wb{};

    IFIDRegister next_if_id{};
    IDEXRegister next_id_ex{};
    EXMEMRegister next_ex_mem{};
    MEMWBRegister next_mem_wb{};

    void commit() {
        if_id = next_if_id;
        id_ex = next_id_ex;
        ex_mem = next_ex_mem;
        mem_wb = next_mem_wb;
    }

    void clear_next() {
        next_if_id.clear();
        next_id_ex.clear();
        next_ex_mem.clear();
        next_mem_wb.clear();
    }

    void clear_all() {
        if_id.clear();
        id_ex.clear();
        ex_mem.clear();
        mem_wb.clear();
        clear_next();
    }
};

}  // namespace rv32i
