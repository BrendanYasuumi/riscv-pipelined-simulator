#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "config.hpp"

namespace rv32i {

struct ExecutionStats {
    uint64_t clock_cycles = 0;
    uint64_t instruction_count = 0;
    uint64_t stall_cycles = 0;
    uint64_t branch_mispredictions = 0;

    double cpi() const;
    double ipc() const;
};

class CPU {
public:
    static constexpr std::size_t kNumRegisters = 32;
    static constexpr uint32_t kResetPC = 0;

    explicit CPU(std::size_t memory_size_bytes, Config config = {});

    void reset();

    const Config& config() const;
    Config& mutable_config();

    uint32_t pc() const;
    void set_pc(uint32_t next_pc);
    void advance_pc(uint32_t bytes = 4);

    uint32_t read_reg(uint8_t reg_index) const;
    void write_reg(uint8_t reg_index, uint32_t value);

    uint8_t read_u8(uint32_t address) const;
    uint16_t read_u16(uint32_t address) const;
    uint32_t read_u32(uint32_t address) const;

    void write_u8(uint32_t address, uint8_t value);
    void write_u16(uint32_t address, uint16_t value);
    void write_u32(uint32_t address, uint32_t value);

    void load_program(const std::vector<uint8_t>& program,
                      uint32_t base_address = 0);

    std::size_t memory_size() const;
    const std::vector<uint8_t>& memory() const;
    std::vector<uint8_t>& mutable_memory();

    const ExecutionStats& stats() const;
    ExecutionStats& mutable_stats();
    void tick();

private:
    void validate_register_index(uint8_t reg_index) const;
    void validate_memory_access(uint32_t address, std::size_t width) const;

    Config config_;
    std::array<uint32_t, kNumRegisters> regs_{};
    uint32_t pc_ = kResetPC;
    std::vector<uint8_t> memory_;
    ExecutionStats stats_{};
};

}  // namespace rv32i
