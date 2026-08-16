#pragma once

#include "cpu.hpp"
#include "pipeline_registers.hpp"

namespace rv32i {

struct StageControl {
    bool flush_id_ex = false;
    bool stall_fetch_decode = false;
    bool stall_pipeline = false;
    bool stop_fetch = false;
};

void stage_WB(CPU& cpu, PipelineRegisters& pipeline);
void stage_MEM(CPU& cpu, PipelineRegisters& pipeline, StageControl& control);
void stage_EX(CPU& cpu, PipelineRegisters& pipeline, StageControl& control);
void stage_ID(CPU& cpu,
              PipelineRegisters& pipeline,
              StageControl& control);
void stage_IF(CPU& cpu, PipelineRegisters& pipeline);

void run_pipeline_cycle(CPU& cpu, PipelineRegisters& pipeline);

}  // namespace rv32i
