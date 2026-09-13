#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "config.hpp"

namespace rv32i {

enum class SaturatingCounterState : uint8_t {
    StronglyNotTaken = 0,
    WeaklyNotTaken = 1,
    WeaklyTaken = 2,
    StronglyTaken = 3
};

class BranchPredictor {
public:
    static constexpr std::size_t kEntryCount = 64;

    BranchPredictor();

    void reset();
    bool predict(uint32_t branch_pc, BranchPredictorType type) const;
    void update(uint32_t branch_pc,
                bool actual_taken,
                BranchPredictorType type);

    SaturatingCounterState counter_state(uint32_t branch_pc) const;

private:
    static std::size_t index_for(uint32_t branch_pc);

    std::array<uint8_t, kEntryCount> counters_{};
};

const char* branch_predictor_type_name(BranchPredictorType type);

}  // namespace rv32i
