#pragma once

#include <iosfwd>

#include "cpu.hpp"
#include "pipeline_registers.hpp"

namespace rv32i {

void print_pipeline_trace(std::ostream& output,
                          const CPU& cpu,
                          const PipelineRegisters& pipeline);
void write_pipeline_trace_csv_header(std::ostream& output);
void write_pipeline_trace_csv_row(std::ostream& output,
                                  const CPU& cpu,
                                  const PipelineRegisters& pipeline);

}  // namespace rv32i
