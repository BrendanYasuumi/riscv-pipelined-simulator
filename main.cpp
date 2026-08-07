#include <iostream>
#include <vector>
#include <cstdint>
#include <iomanip>

struct CPU {
    uint32_t regs[32] = {0};     // 32 general-purpose registers (x0 to x31)
    uint32_t pc = 0;             // Program Counter
    std::vector<uint8_t> memory; // Virtual RAM

    CPU(size_t mem_size) : memory(mem_size, 0) {}

    void write_reg(int reg, uint32_t val) {
        if (reg != 0) regs[reg] = val;
    }

    uint32_t read_reg(int reg) const {
        if (reg == 0) return 0;
        return regs[reg];
    }
};

// 1. FETCH: Reconstructs 4 bytes into a single 32-bit instruction (Little-Endian)
uint32_t fetch(const CPU& cpu) {
    return static_cast<uint32_t>(cpu.memory[cpu.pc])       |
          (static_cast<uint32_t>(cpu.memory[cpu.pc + 1]) << 8)  |
          (static_cast<uint32_t>(cpu.memory[cpu.pc + 2]) << 16) |
          (static_cast<uint32_t>(cpu.memory[cpu.pc + 3]) << 24);
}

// 2. DECODE & EXECUTE
void decode_and_execute(CPU& cpu, uint32_t inst) {
    uint32_t opcode = inst & 0x7F;
    uint32_t rd     = (inst >> 7) & 0x1F;
    uint32_t funct3 = (inst >> 12) & 0x7;
    uint32_t rs1    = (inst >> 15) & 0x1F;
    uint32_t rs2    = (inst >> 20) & 0x1F;
    uint32_t funct7 = (inst >> 25) & 0x7F;

    std::cout << "--- Decoded Fields ---" << std::endl;
    std::cout << "Opcode: 0x" << std::hex << opcode << " (Base 10: " << std::dec << opcode << ")" << std::endl;
    std::cout << "rd:     x" << rd << std::endl;
    std::cout << "rs1:    x" << rs1 << std::endl;
    std::cout << "rs2:    x" << rs2 << std::endl;
    std::cout << "funct3: 0x" << std::hex << funct3 << std::endl;
    std::cout << "funct7: 0x" << std::hex << funct7 << std::dec << std::endl;

    // Check if opcode matches R-Type (0x33)
    if (opcode == 0x33) {
        uint32_t val1 = cpu.read_reg(rs1);
        uint32_t val2 = cpu.read_reg(rs2);

        if (funct3 == 0x0 && funct7 == 0x00) { // ADD
            uint32_t result = val1 + val2;
            cpu.write_reg(rd, result);
            std::cout << "\n[EXECUTE] ADD x" << rd << ", x" << rs1 << ", x" << rs2 << std::endl;
            std::cout << "Result: " << val1 << " + " << val2 << " = " << result << " saved to x" << rd << std::endl;
        }
    }
}

int main() {
    CPU cpu(1024);

    // Pre-load test values into input registers x1 and x2
    cpu.write_reg(1, 15); // x1 = 15
    cpu.write_reg(2, 25); // x2 = 25

    // Load the 32-bit machine code for "ADD x3, x1, x2" (0x002081B3) into RAM at address 0 (Little-Endian order)
    cpu.memory[0] = 0xB3;
    cpu.memory[1] = 0x81;
    cpu.memory[2] = 0x20;
    cpu.memory[3] = 0x00;

    std::cout << "Initial Register Values:" << std::endl;
    std::cout << "x1 = " << cpu.read_reg(1) << ", x2 = " << cpu.read_reg(2) << ", x3 = " << cpu.read_reg(3) << "\n\n";

    // Run Fetch -> Decode -> Execute
    uint32_t instruction = fetch(cpu);
    decode_and_execute(cpu, instruction);

    std::cout << "\nFinal Register Values:" << std::endl;
    std::cout << "x1 = " << cpu.read_reg(1) << ", x2 = " << cpu.read_reg(2) << ", x3 = " << cpu.read_reg(3) << std::endl;

    return 0;
}