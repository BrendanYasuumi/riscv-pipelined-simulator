#pragma once

#include "instruction.hpp"
#include "pipeline_registers.hpp"

namespace rv32i {

struct SourceRegisters {
    bool uses_rs1 = false;
    bool uses_rs2 = false;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
};

struct HazardDecision {
    bool stall = false;
};

SourceRegisters get_source_registers(const DecodedInstruction& instruction);
HazardDecision detect_raw_hazard(const PipelineRegisters& pipeline);

}  // namespace rv32i
