#pragma once //Tells compiler to load this header file only one time during a build

namespace rv32i {

struct Config {
    // Enables common bypass paths such as EX/MEM -> EX and MEM/WB -> EX. The
    // CLI keeps this enabled so normal project runs stay simple.
    bool enable_forwarding = true;
};

}  // namespace rv32i
