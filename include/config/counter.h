#pragma once
#include "utility/tagged.h"
#include <cstddef>
#include <string_view>

namespace dark::weight {

#define register_class(name, weight, ...)                                                          \
    struct Counter##name {                                                                         \
        std::size_t w##name{};                                                                     \
        static constexpr std::size_t kDefaultWeight = weight;                                      \
        static constexpr std::string_view kName     = #name;                                       \
        static_assert(weight != 0, "Weight must be greater than 0");                               \
        static constexpr std::string_view kMembers[] = {__VA_ARGS__};                              \
        auto get_weight() const -> std::size_t {                                                   \
            return this->w##name;                                                                  \
        }                                                                                          \
        auto set_weight(std::size_t w) -> void {                                                   \
            this->w##name = w;                                                                     \
        }                                                                                          \
    }

register_class(Arith, 1, "add", "sub");
register_class(Upper, 1, "lui", "auipc");
register_class(Compare, 1, "slt", "sltu");
register_class(Shift, 1, "sll", "srl", "sra");
register_class(Bitwise, 1, "and", "or", "xor");
register_class(Branch, 10, "beq", "bne", "blt", "bge", "bltu", "bgeu");
register_class(Load, 64, "lb", "lh", "lw", "lbu", "lhu");
register_class(Store, 64, "sb", "sh", "sw");
register_class(Multiply, 4, "mul", "mulh", "mulhsu", "mulhu");
register_class(Divide, 20, "div", "divu", "rem", "remu");
register_class(Jal, 1, "jal");
register_class(Jalr, 2, "jalr");

// External devices

register_class(PredictTaken, 2, "");
register_class(CacheLoad, 4, "");
register_class(CacheStore, 4, "");

#undef register_class

struct Counter
    : tagged<
          CounterArith, CounterUpper, CounterCompare, CounterShift, CounterBitwise, CounterBranch,
          CounterLoad, CounterStore, CounterMultiply, CounterDivide, CounterJal, CounterJalr,
          CounterPredictTaken, CounterCacheLoad, CounterCacheStore> {};

} // namespace dark::weight
