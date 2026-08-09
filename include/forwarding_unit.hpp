#pragma once

#include "pipeline_registers.hpp"

namespace rv32i {

struct ForwardedOperands {
    uint32_t rs1_value = 0;
    uint32_t rs2_value = 0;
    bool forwarded_rs1 = false;
    bool forwarded_rs2 = false;
};

ForwardedOperands resolve_forwarding(const IDEXRegister& id_ex,
                                     const PipelineRegisters& pipeline,
                                     bool enable_forwarding);

}  // namespace rv32i
