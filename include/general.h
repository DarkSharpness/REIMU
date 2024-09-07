#pragma once
#include <cstdint>

// Some general operation behavior for RISC-V

namespace dark::general {

enum class ArithOp : std::uint8_t {
    // Normal arithmetic
    ADD,
    SUB,
    AND,
    OR,
    XOR,
    SLL,
    SRL,
    SRA,
    SLT,
    SLTU,
    // Multiplication and division extension
    MUL,
    MULH,
    MULHSU,
    MULHU,
    DIV,
    DIVU,
    REM,
    REMU
};

enum class MemoryOp : std::uint8_t { LB, LH, LW, LBU, LHU, SB, SH, SW };

enum class BranchOp : std::uint8_t { BEQ, BNE, BLT, BGE, BLTU, BGEU };

} // namespace dark::general
