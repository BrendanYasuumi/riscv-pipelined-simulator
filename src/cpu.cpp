#include "cpu.hpp"

#include <algorithm>
#include <stdexcept>

namespace rv32i {

double ExecutionStats::cpi() const {
    if (instruction_count == 0) {
        return 0.0;
    }
    return static_cast<double>(clock_cycles) /
           static_cast<double>(instruction_count);
}

double ExecutionStats::ipc() const {
    if (clock_cycles == 0) {
        return 0.0;
    }
    return static_cast<double>(instruction_count) /
           static_cast<double>(clock_cycles);
}

CPU::CPU(std::size_t memory_size_bytes, Config config)
    : config_(config), memory_(memory_size_bytes, 0) {
    branch_predictor_counters_.fill(1);
}

void CPU::reset() {
    regs_.fill(0);
    branch_predictor_counters_.fill(1);
    pc_ = kResetPC;
    halted_ = false;
    stats_ = ExecutionStats{};
}

const Config& CPU::config() const {
    return config_;
}

Config& CPU::mutable_config() {
    return config_;
}

uint32_t CPU::pc() const {
    return pc_;
}

void CPU::set_pc(uint32_t next_pc) {
    pc_ = next_pc;
}

void CPU::advance_pc(uint32_t bytes) {
    pc_ += bytes;
}

uint32_t CPU::read_reg(uint8_t reg_index) const {
    validate_register_index(reg_index);

    // RISC-V x0 is hardwired to zero. Reads must ignore any stored value.
    if (reg_index == 0) {
        return 0;
    }

    return regs_[reg_index];
}

void CPU::write_reg(uint8_t reg_index, uint32_t value) {
    validate_register_index(reg_index);

    // Writes to x0 retire architecturally, but the state never changes.
    if (reg_index == 0) {
        return;
    }

    regs_[reg_index] = value;
}

uint8_t CPU::read_u8(uint32_t address) const {
    validate_memory_access(address, sizeof(uint8_t));
    return memory_[address];
}

uint16_t CPU::read_u16(uint32_t address) const {
    validate_memory_access(address, sizeof(uint16_t));

    return static_cast<uint16_t>(memory_[address]) |
           static_cast<uint16_t>(memory_[address + 1] << 8);
}

uint32_t CPU::read_u32(uint32_t address) const {
    validate_memory_access(address, sizeof(uint32_t));

    // RV32I instructions and data words are assembled little-endian.
    return static_cast<uint32_t>(memory_[address]) |
           (static_cast<uint32_t>(memory_[address + 1]) << 8) |
           (static_cast<uint32_t>(memory_[address + 2]) << 16) |
           (static_cast<uint32_t>(memory_[address + 3]) << 24);
}

void CPU::write_u8(uint32_t address, uint8_t value) {
    validate_memory_access(address, sizeof(uint8_t));
    memory_[address] = value;
}

void CPU::write_u16(uint32_t address, uint16_t value) {
    validate_memory_access(address, sizeof(uint16_t));

    memory_[address] = static_cast<uint8_t>(value & 0xFFu);
    memory_[address + 1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
}

void CPU::write_u32(uint32_t address, uint32_t value) {
    validate_memory_access(address, sizeof(uint32_t));

    memory_[address] = static_cast<uint8_t>(value & 0xFFu);
    memory_[address + 1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
    memory_[address + 2] = static_cast<uint8_t>((value >> 16) & 0xFFu);
    memory_[address + 3] = static_cast<uint8_t>((value >> 24) & 0xFFu);
}

void CPU::load_program(const std::vector<uint8_t>& program,
                       uint32_t base_address) {
    if (program.empty()) {
        return;
    }

    validate_memory_access(base_address, program.size());
    std::copy(program.begin(), program.end(), memory_.begin() + base_address);
    pc_ = base_address;
    halted_ = false;
}

std::size_t CPU::memory_size() const {
    return memory_.size();
}

const std::vector<uint8_t>& CPU::memory() const {
    return memory_;
}

std::vector<uint8_t>& CPU::mutable_memory() {
    return memory_;
}

const ExecutionStats& CPU::stats() const {
    return stats_;
}

ExecutionStats& CPU::mutable_stats() {
    return stats_;
}

void CPU::tick() {
    ++stats_.clock_cycles;
}

bool CPU::halted() const {
    return halted_;
}

void CPU::halt() {
    halted_ = true;
}

bool CPU::predict_branch(uint32_t pc) const {
    switch (config_.branch_predictor_type) {
        case BranchPredictorType::AlwaysNotTaken:
            return false;
        case BranchPredictorType::AlwaysTaken:
            return true;
        case BranchPredictorType::TwoBitSaturatingCounter: {
            const std::size_t index =
                (pc >> 2) % branch_predictor_counters_.size();
            return branch_predictor_counters_[index] >= 2;
        }
    }

    return false;
}

void CPU::update_branch_predictor(uint32_t pc, bool taken) {
    if (config_.branch_predictor_type !=
        BranchPredictorType::TwoBitSaturatingCounter) {
        return;
    }

    const std::size_t index = (pc >> 2) % branch_predictor_counters_.size();
    uint8_t& counter = branch_predictor_counters_[index];

    if (taken && counter < 3) {
        ++counter;
    } else if (!taken && counter > 0) {
        --counter;
    }
}

void CPU::validate_register_index(uint8_t reg_index) const {
    if (reg_index >= kNumRegisters) {
        throw std::out_of_range("register index must be in range x0..x31");
    }
}

void CPU::validate_memory_access(uint32_t address, std::size_t width) const {
    if (width == 0) {
        return;
    }

    const std::size_t start = static_cast<std::size_t>(address);
    if (start > memory_.size() || width > memory_.size() - start) {
        throw std::out_of_range("memory access exceeds simulated RAM bounds");
    }
}

}  // namespace rv32i
