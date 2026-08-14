#include "state_dump.hpp"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <stdexcept>

namespace rv32i {

void print_register_dump(std::ostream& output, const CPU& cpu) {
    output << "\nRegister dump:\n";

    for (uint8_t reg = 0; reg < 32; ++reg) {
        const uint32_t value = cpu.read_reg(reg);
        output << "  x" << std::dec << std::setw(2) << std::setfill('0')
               << static_cast<int>(reg) << " = 0x" << std::hex
               << std::setw(8) << std::setfill('0') << value << std::dec
               << std::setfill(' ') << " (" << value << ")\n";
    }
}

void print_memory_dump(std::ostream& output,
                       const CPU& cpu,
                       uint32_t start_address,
                       uint32_t byte_count) {
    const uint64_t end_address =
        static_cast<uint64_t>(start_address) + byte_count;
    if (end_address > cpu.memory_size()) {
        throw std::out_of_range(
            "memory dump range exceeds simulated RAM bounds");
    }

    output << "\nMemory dump [0x" << std::hex << std::setw(8)
           << std::setfill('0') << start_address << "..0x" << std::setw(8)
           << end_address << std::dec << std::setfill(' ') << "):\n";

    for (uint32_t offset = 0; offset < byte_count; offset += 16) {
        const uint32_t address = start_address + offset;
        output << "  0x" << std::hex << std::setw(8) << std::setfill('0')
               << address << ":";

        const uint32_t bytes_this_line =
            std::min<uint32_t>(16, byte_count - offset);
        for (uint32_t i = 0; i < bytes_this_line; ++i) {
            output << ' ' << std::setw(2) << std::setfill('0')
                   << static_cast<uint32_t>(cpu.read_u8(address + i));
        }

        output << std::dec << std::setfill(' ') << '\n';
    }
}

}  // namespace rv32i
