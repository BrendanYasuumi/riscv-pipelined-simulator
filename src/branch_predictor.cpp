#include "branch_predictor.hpp"

namespace rv32i {

namespace {

constexpr uint8_t kWeaklyNotTaken =
    static_cast<uint8_t>(SaturatingCounterState::WeaklyNotTaken);
constexpr uint8_t kWeaklyTaken =
    static_cast<uint8_t>(SaturatingCounterState::WeaklyTaken);
constexpr uint8_t kStronglyTaken =
    static_cast<uint8_t>(SaturatingCounterState::StronglyTaken);

}  // namespace

BranchPredictor::BranchPredictor() {
    reset();
}

void BranchPredictor::reset() {
    counters_.fill(kWeaklyNotTaken);
}

bool BranchPredictor::predict(uint32_t branch_pc,
                              BranchPredictorType type) const {
    switch (type) {
        case BranchPredictorType::AlwaysNotTaken:
            return false;
        case BranchPredictorType::AlwaysTaken:
            return true;
        case BranchPredictorType::TwoBitSaturating:
            return counters_[index_for(branch_pc)] >= kWeaklyTaken;
    }

    return false;
}

void BranchPredictor::update(uint32_t branch_pc,
                             bool actual_taken,
                             BranchPredictorType type) {
    if (type != BranchPredictorType::TwoBitSaturating) {
        return;
    }

    uint8_t& counter = counters_[index_for(branch_pc)];
    if (actual_taken) {
        if (counter < kStronglyTaken) {
            ++counter;
        }
    } else if (counter > 0) {
        --counter;
    }
}

SaturatingCounterState BranchPredictor::counter_state(
    uint32_t branch_pc) const {
    return static_cast<SaturatingCounterState>(
        counters_[index_for(branch_pc)]);
}

std::size_t BranchPredictor::index_for(uint32_t branch_pc) {
    return (branch_pc >> 2) % kEntryCount;
}

const char* branch_predictor_type_name(BranchPredictorType type) {
    switch (type) {
        case BranchPredictorType::AlwaysNotTaken:
            return "always-not-taken";
        case BranchPredictorType::AlwaysTaken:
            return "always-taken";
        case BranchPredictorType::TwoBitSaturating:
            return "two-bit";
    }

    return "unknown";
}

}  // namespace rv32i
