#pragma once
#include <cstddef>

namespace dark::config {

struct CounterArith {
    std::size_t add     {};
    std::size_t sub     {};
    std::size_t lui     {};
    std::size_t slt     {};
    std::size_t sltu    {};
    std::size_t auipc   {};
};

struct CounterBitwise {
    std::size_t and_    {};
    std::size_t or_     {};
    std::size_t xor_    {};
    std::size_t sll     {};
    std::size_t srl     {};
    std::size_t sra     {};
};

struct CounterBranch {
    std::size_t beq     {};
    std::size_t bne     {};
    std::size_t blt     {};
    std::size_t bge     {};
    std::size_t bltu    {};
    std::size_t bgeu    {};
};

struct CounterMemory {
    std::size_t lb      {};
    std::size_t lh      {};
    std::size_t lw      {};
    std::size_t lbu     {};
    std::size_t lhu     {};
    std::size_t sb      {};
    std::size_t sh      {};
    std::size_t sw      {};
};

struct CounterMultiply {
    std::size_t mul     {};
    std::size_t mulh    {};
    std::size_t mulhsu  {};
    std::size_t mulhu   {};
};

struct CounterDivide {
    std::size_t div     {};
    std::size_t divu    {};
    std::size_t rem     {};
    std::size_t remu    {};
};

struct CounterJump {
    std::size_t jal     {};
    std::size_t jalr    {};
};

struct Counter :
    CounterArith,
    CounterBitwise,
    CounterBranch,
    CounterMemory,
    CounterMultiply,
    CounterDivide,
    CounterJump
{};

} // namespace dark
