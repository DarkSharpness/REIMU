#pragma once
#include <cstdint>
#include <string_view>

namespace dark {

// RISC-V standard registers
enum class Register : std::uint8_t {
    zero = 0,
    ra   = 1,
    sp   = 2,
    gp   = 3,
    tp   = 4,
    t0   = 5,
    t1   = 6,
    t2   = 7,
    s0   = 8,
    s1   = 9,
    a0   = 10,
    a1   = 11,
    a2   = 12,
    a3   = 13,
    a4   = 14,
    a5   = 15,
    a6   = 16,
    a7   = 17,
    s2   = 18,
    s3   = 19,
    s4   = 20,
    s5   = 21,
    s6   = 22,
    s7   = 23,
    s8   = 24,
    s9   = 25,
    s10  = 26,
    s11  = 27,
    t3   = 28,
    t4   = 29,
    t5   = 30,
    t6   = 31,

    // Floating point part.
    ft0  = 32,
    ft1  = 33,
    ft2  = 34,
    ft3  = 35,
    ft4  = 36,
    ft5  = 37,
    ft6  = 38,
    ft7  = 39,
    fs0  = 40,
    fs1  = 41,
    fa0  = 42,
    fa1  = 43,
    fa2  = 44,
    fa3  = 45,
    fa4  = 46,
    fa5  = 47,
    fa6  = 48,
    fa7  = 49,
    fs2  = 50,
    fs3  = 51,
    fs4  = 52,
    fs5  = 53,
    fs6  = 54,
    fs7  = 55,
    fs8  = 56,
    fs9  = 57,
    fs10 = 58,
    fs11 = 59,
    ft8  = 60,
    ft9  = 61,
    ft10 = 62,
    ft11 = 63,
};

auto reg_to_sv(Register) -> std::string_view;

static constexpr auto reg_to_int(Register reg) -> std::uint32_t {
    return static_cast<std::uint32_t>(reg);
}

static constexpr auto freg_to_int(Register reg) -> std::uint32_t {
    return reg_to_int(reg) - 32;
}

static constexpr auto int_to_reg(std::uint32_t reg) -> Register {
    return static_cast<Register>(reg);
}

static constexpr auto int_to_freg(std::uint32_t reg) -> Register {
    return int_to_reg(reg + 32);
}

static constexpr auto is_int_reg(Register reg) -> bool {
    return reg <= Register::t6;
}

static constexpr auto is_fp_reg(Register reg) -> bool {
    return reg >= Register::ft0;
}

} // namespace dark
