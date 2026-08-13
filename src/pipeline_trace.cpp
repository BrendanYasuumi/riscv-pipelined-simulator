#include "pipeline_trace.hpp"

#include "instruction.hpp"

#include <iomanip>
#include <ostream>

namespace rv32i {

namespace {

const char* latch_name(bool valid, uint32_t instruction) {
    if (!valid) {
        return "bubble";
    }

    return instruction_kind_name(decode_instruction(instruction).kind);
}

void write_csv_latch(std::ostream& output, bool valid, uint32_t instruction) {
    output << (valid ? latch_name(valid, instruction) : "bubble");
}

void print_latch(std::ostream& output,
                 const char* name,
                 bool valid,
                 uint32_t pc,
                 uint32_t instruction) {
    output << "  " << std::left << std::setw(6) << name << " ";

    if (!valid) {
        output << "bubble\n";
        return;
    }

    output << std::right << "pc=0x" << std::hex << std::setw(8)
           << std::setfill('0') << pc << " inst=0x" << std::setw(8)
           << instruction << std::dec
           << std::setfill(' ') << " op=" << latch_name(valid, instruction)
           << '\n';
}

}  // namespace

void print_pipeline_trace(std::ostream& output,
                          const CPU& cpu,
                          const PipelineRegisters& pipeline) {
    output << "\nCycle " << cpu.stats().clock_cycles << ":\n";
    print_latch(output,
                "IF/ID",
                pipeline.if_id.valid,
                pipeline.if_id.pc,
                pipeline.if_id.instruction);
    print_latch(output,
                "ID/EX",
                pipeline.id_ex.valid,
                pipeline.id_ex.pc,
                pipeline.id_ex.instruction);
    print_latch(output,
                "EX/MEM",
                pipeline.ex_mem.valid,
                pipeline.ex_mem.pc,
                pipeline.ex_mem.instruction);
    print_latch(output,
                "MEM/WB",
                pipeline.mem_wb.valid,
                pipeline.mem_wb.pc,
                pipeline.mem_wb.instruction);
}

void write_pipeline_trace_csv_header(std::ostream& output) {
    output << "cycle,pc,if_id,id_ex,ex_mem,mem_wb,retired,stalls,"
              "branch_mispredictions\n";
}

void write_pipeline_trace_csv_row(std::ostream& output,
                                  const CPU& cpu,
                                  const PipelineRegisters& pipeline) {
    output << cpu.stats().clock_cycles << ',' << cpu.pc() << ',';
    write_csv_latch(output, pipeline.if_id.valid, pipeline.if_id.instruction);
    output << ',';
    write_csv_latch(output, pipeline.id_ex.valid, pipeline.id_ex.instruction);
    output << ',';
    write_csv_latch(output, pipeline.ex_mem.valid, pipeline.ex_mem.instruction);
    output << ',';
    write_csv_latch(output, pipeline.mem_wb.valid, pipeline.mem_wb.instruction);
    output << ',' << cpu.stats().instruction_count << ','
           << cpu.stats().stall_cycles << ','
           << cpu.stats().branch_mispredictions << '\n';
}

}  // namespace rv32i
