#include "riscv/register.h"
#include "utility/cast.h"
#include "utility/hash.h"
#include <optional>
#include <string_view>

namespace dark {

// clang-format off
static constexpr std::string_view kRegisterMap[] = {
    // Integer registers
    "zero", "ra", "sp", "gp",   "tp", "t0", "t1", "t2",
    "s0",   "s1", "a0", "a1",   "a2", "a3", "a4", "a5",
    "a6",   "a7", "s2", "s3",   "s4", "s5", "s6", "s7",
    "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6",

    // Floating point registers
    "ft0",  "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7",
    "fs0",  "fs1", "fa0", "fa1", "fa2", "fa3", "fa4", "fa5",
    "fa6",  "fa7", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7",
    "fs8",  "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11",
};
// clang-format on

auto reg_to_sv(Register reg) -> std::string_view {
    return kRegisterMap[reg_to_int(reg)];
}

auto sv_to_reg_nothrow(std::string_view view) noexcept -> std::optional<Register> {
    using hash::switch_hash_impl;
#define match_or_break(expr)                                                                       \
    case switch_hash_impl(#expr):                                                                  \
        if (view == #expr) {                                                                       \
            return Register::expr;                                                                 \
        }                                                                                          \
        break

    switch (switch_hash_impl(view)) {
        match_or_break(zero);
        match_or_break(ra);
        match_or_break(sp);
        match_or_break(gp);
        match_or_break(tp);
        match_or_break(t0);
        match_or_break(t1);
        match_or_break(t2);
        match_or_break(s0);
        match_or_break(s1);
        match_or_break(a0);
        match_or_break(a1);
        match_or_break(a2);
        match_or_break(a3);
        match_or_break(a4);
        match_or_break(a5);
        match_or_break(a6);
        match_or_break(a7);
        match_or_break(s2);
        match_or_break(s3);
        match_or_break(s4);
        match_or_break(s5);
        match_or_break(s6);
        match_or_break(s7);
        match_or_break(s8);
        match_or_break(s9);
        match_or_break(s10);
        match_or_break(s11);
        match_or_break(t3);
        match_or_break(t4);
        match_or_break(t5);
        match_or_break(t6);
        match_or_break(ft0);
        match_or_break(ft1);
        match_or_break(ft2);
        match_or_break(ft3);
        match_or_break(ft4);
        match_or_break(ft5);
        match_or_break(ft6);
        match_or_break(ft7);
        match_or_break(fs0);
        match_or_break(fs1);
        match_or_break(fa0);
        match_or_break(fa1);
        match_or_break(fa2);
        match_or_break(fa3);
        match_or_break(fa4);
        match_or_break(fa5);
        match_or_break(fa6);
        match_or_break(fa7);
        match_or_break(fs2);
        match_or_break(fs3);
        match_or_break(fs4);
        match_or_break(fs5);
        match_or_break(fs6);
        match_or_break(fs7);
        match_or_break(fs8);
        match_or_break(fs9);
        match_or_break(fs10);
        match_or_break(fs11);
        match_or_break(ft8);
        match_or_break(ft9);
        match_or_break(ft10);
        match_or_break(ft11);
        default: break;
    }

#undef match_or_break

    if (view.empty())
        return std::nullopt;

    if (view.starts_with("x")) {
        auto num = sv_to_integer<std::size_t>(view.substr(1));
        if (num.has_value() && *num < 32)
            return static_cast<Register>(*num);
    } else if (view.starts_with("f")) {
        auto num = sv_to_integer<std::size_t>(view.substr(1));
        if (num.has_value() && *num < 32)
            return static_cast<Register>(*num + 32);
    }

    if (view == "fp") {
        return Register::s0;
    }

    return std::nullopt;
}


} // namespace dark
