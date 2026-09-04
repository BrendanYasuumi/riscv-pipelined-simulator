#include "state_dump.hpp"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace rv32i {

namespace {

std::vector<MemoryDumpRange> normalize_memory_ranges(
    const CPU& cpu,
    const std::vector<MemoryDumpRange>& requested_ranges) {
    std::vector<bool> selected(cpu.memory_size(), false);

    if (requested_ranges.empty()) {
        for (const MemoryWrite& write : cpu.memory_writes()) {
            const uint64_t end =
                static_cast<uint64_t>(write.address) + write.byte_count;
            if (end > cpu.memory_size()) {
                throw std::out_of_range(
                    "recorded memory write exceeds simulated RAM bounds");
            }

            for (uint32_t address = write.address; address < end; ++address) {
                selected[address] = true;
            }
        }
    } else {
        for (const MemoryDumpRange& range : requested_ranges) {
            const uint64_t end =
                static_cast<uint64_t>(range.start_address) + range.byte_count;
            if (end > cpu.memory_size()) {
                throw std::out_of_range(
                    "architectural-state memory range exceeds simulated RAM bounds");
            }

            for (uint32_t address = range.start_address; address < end;
                 ++address) {
                selected[address] = true;
            }
        }
    }

    std::vector<MemoryDumpRange> normalized;
    std::size_t address = 0;
    while (address < selected.size()) {
        while (address < selected.size() && !selected[address]) {
            ++address;
        }
        if (address == selected.size()) {
            break;
        }

        const std::size_t start = address;
        while (address < selected.size() && selected[address]) {
            ++address;
        }

        normalized.push_back(
            {static_cast<uint32_t>(start),
             static_cast<uint32_t>(address - start)});
    }

    return normalized;
}

void print_hex32(std::ostream& output, uint32_t value) {
    output << "0x" << std::hex << std::setw(8) << std::setfill('0') << value
           << std::dec << std::setfill(' ');
}

}  // namespace

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

void print_written_memory_dump(std::ostream& output, const CPU& cpu) {
    output << "\nWritten memory dump:\n";

    if (cpu.memory_writes().empty()) {
        output << "  <no memory writes>\n";
        return;
    }

    std::vector<bool> touched(cpu.memory_size(), false);
    for (const MemoryWrite& write : cpu.memory_writes()) {
        const uint64_t end =
            static_cast<uint64_t>(write.address) + write.byte_count;
        if (end > cpu.memory_size()) {
            throw std::out_of_range(
                "recorded memory write exceeds simulated RAM bounds");
        }

        for (uint32_t address = write.address; address < end; ++address) {
            touched[address] = true;
        }
    }

    std::size_t address = 0;
    while (address < touched.size()) {
        while (address < touched.size() && !touched[address]) {
            ++address;
        }
        if (address == touched.size()) {
            break;
        }

        const std::size_t range_start = address;
        while (address < touched.size() && touched[address]) {
            ++address;
        }
        const std::size_t range_end = address;

        output << "  Range [0x" << std::hex << std::setw(8)
               << std::setfill('0') << range_start << "..0x"
               << std::setw(8) << range_end << std::dec
               << std::setfill(' ') << "):\n";

        for (std::size_t row = range_start; row < range_end; row += 16) {
            output << "    0x" << std::hex << std::setw(8)
                   << std::setfill('0') << row << ":";

            const std::size_t bytes_this_line =
                std::min<std::size_t>(16, range_end - row);
            for (std::size_t i = 0; i < bytes_this_line; ++i) {
                output << ' ' << std::setw(2) << std::setfill('0')
                       << static_cast<uint32_t>(
                              cpu.read_u8(static_cast<uint32_t>(row + i)));
            }

            output << std::dec << std::setfill(' ') << '\n';
        }
    }
}

void print_architectural_state_json(
    std::ostream& output,
    const CPU& cpu,
    const std::vector<MemoryDumpRange>& memory_ranges) {
    const std::vector<MemoryDumpRange> normalized_ranges =
        normalize_memory_ranges(cpu, memory_ranges);

    output << "{\n";
    output << "  \"schema\": \"rv32i-architectural-state-v1\",\n";
    output << "  \"pc\": \"";
    print_hex32(output, cpu.pc());
    output << "\",\n";
    output << "  \"halted\": " << (cpu.halted() ? "true" : "false")
           << ",\n";
    output << "  \"registers\": {\n";

    for (uint8_t reg = 0; reg < CPU::kNumRegisters; ++reg) {
        output << "    \"x" << std::dec << std::setw(2) << std::setfill('0')
               << static_cast<int>(reg) << "\": \"";
        print_hex32(output, cpu.read_reg(reg));
        output << "\"" << (reg + 1 == CPU::kNumRegisters ? "\n" : ",\n");
    }

    output << "  },\n";
    output << "  \"memory\": [";
    if (!normalized_ranges.empty()) {
        output << '\n';
    }

    for (std::size_t index = 0; index < normalized_ranges.size(); ++index) {
        const MemoryDumpRange& range = normalized_ranges[index];
        output << "    {\"start\": \"";
        print_hex32(output, range.start_address);
        output << "\", \"length\": " << range.byte_count
               << ", \"bytes\": \"";

        for (uint32_t offset = 0; offset < range.byte_count; ++offset) {
            output << std::hex << std::setw(2) << std::setfill('0')
                   << static_cast<uint32_t>(
                          cpu.read_u8(range.start_address + offset));
        }

        output << std::dec << std::setfill(' ') << "\"}"
               << (index + 1 == normalized_ranges.size() ? "\n" : ",\n");
    }

    output << "  ]\n";
    output << "}\n";
}

}  // namespace rv32i
