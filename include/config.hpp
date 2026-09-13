#pragma once //Tells compiler to load this header file only one time during a build

namespace rv32i {

enum class BranchPredictorType {
    AlwaysNotTaken,
    AlwaysTaken,
    TwoBitSaturating
};

struct Config {
    // Enables common bypass paths such as EX/MEM -> EX and MEM/WB -> EX. The
    // CLI keeps this enabled so normal project runs stay simple.
    bool enable_forwarding = true;

    // A dynamic predictor is the default microarchitecture. Static modes remain
    // available as baselines for measuring prediction behavior.
    BranchPredictorType branch_predictor_type =
        BranchPredictorType::TwoBitSaturating;
};

}  // namespace rv32i
