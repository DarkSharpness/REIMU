#pragma once
#include "assembly/forward.h"
#include "assembly/storage/immediate.h"
#include "utility/ustring.h"
#include <vector>

// Immediate

namespace dark {

struct IntImmediate : ImmediateBase {
    target_size_t data;
    explicit IntImmediate(target_size_t data) : data(data) {}
};

struct StrImmediate : ImmediateBase {
    unique_string data;
    explicit StrImmediate(std::string_view data) : data(data) {}
};

struct RelImmediate : ImmediateBase {
    Immediate imm;
    enum class Operand { HI, LO, PCREL_HI, PCREL_LO } operand;
    explicit RelImmediate(Immediate imm, Operand op) : imm(std::move(imm)), operand(op) {}
};

struct TreeImmediate : ImmediateBase {
    enum class Operator { ADD, SUB, END };
    struct Pair {
        Immediate imm;
        Operator op;
    };
    std::vector<Pair> data;
    explicit TreeImmediate(std::vector<Pair> data) : data(std::move(data)) {}
};

} // namespace dark
