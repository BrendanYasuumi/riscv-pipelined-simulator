#pragma once

#include <cstdint>

namespace rv32i {

enum class BranchPredictorType {
    AlwaysNotTaken,
    AlwaysTaken,
    TwoBitSaturatingCounter
};

struct Config {
    // Enables common bypass paths such as EX/MEM -> EX and MEM/WB -> EX.
    // When disabled, RAW dependencies are resolved by inserting bubbles.
    bool enable_forwarding = true;

    // Branch prediction policy used by the fetch stage. Mispredictions will
    // flush younger instructions and redirect the PC when the branch resolves.
    BranchPredictorType branch_predictor_type =
        BranchPredictorType::AlwaysNotTaken;

    // Number of cycles consumed by a data-memory access. A value of 1 models
    // ideal single-cycle SRAM; larger values model cache misses or slow memory.
    uint32_t memory_latency_cycles = 1;

    // Forward-looking hooks. These are intentionally inert in the first scalar
    // five-stage pipeline, but they keep configuration growth explicit.
    bool enable_superscalar = false;
    bool cache_extension_ready = false;
};

}  // namespace rv32i
