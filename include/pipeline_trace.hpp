#pragma once

#include <iosfwd>

#include "cpu.hpp"
#include "pipeline_registers.hpp"

namespace rv32i {

void print_pipeline_trace(std::ostream& output,
                          const CPU& cpu,
                          const PipelineRegisters& pipeline);

}  // namespace rv32i
