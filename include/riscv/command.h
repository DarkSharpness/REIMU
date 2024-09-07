#pragma once
#include "declarations.h"
#include <bit>
#include <concepts>

namespace dark::command {

namespace __details {

template <typename _Derived>
struct crtp {
    constexpr auto to_integer() const -> command_size_t {
        auto &derived = static_cast<const _Derived &>(*this);
        return std::bit_cast<command_size_t>(derived);
    }
    static constexpr auto from_integer(command_size_t cmd) -> _Derived;
};

template <std::size_t _Len, std::unsigned_integral _Tp>
static constexpr _Tp sign_extend(_Tp value) {
    static_assert(_Len <= sizeof(_Tp) * 8, "N must be less than the size of the type");
    using _Vp = std::make_signed_t<_Tp>;
    struct {
        _Vp x : _Len;
    } s;
    return static_cast<_Tp>(s.x = value);
}

namespace Arith_Funct3 {

enum Funct : command_size_t {
    ADD  = 0b000,
    SLL  = 0b001,
    SLT  = 0b010,
    SLTU = 0b011,
    XOR  = 0b100,
    SRL  = 0b101,
    OR   = 0b110,
    AND  = 0b111,
    SUB  = 0b000,
    SRA  = 0b101,

    MUL    = 0b000,
    MULH   = 0b001,
    MULHSU = 0b010,
    MULHU  = 0b011,
    DIV    = 0b100,
    DIVU   = 0b101,
    REM    = 0b110,
    REMU   = 0b111,
};

} // namespace Arith_Funct3

namespace Arith_Funct7 {

enum Funct : command_size_t {
    ADD  = 0b0000000,
    SLL  = 0b0000000,
    SLT  = 0b0000000,
    SLTU = 0b0000000,
    XOR  = 0b0000000,
    SRL  = 0b0000000,
    OR   = 0b0000000,
    AND  = 0b0000000,
    SUB  = 0b0100000,
    SRA  = 0b0100000,

    MUL    = 0b0000001,
    MULH   = 0b0000001,
    MULHSU = 0b0000001,
    MULHU  = 0b0000001,
    DIV    = 0b0000001,
    DIVU   = 0b0000001,
    REM    = 0b0000001,
    REMU   = 0b0000001,
};

} // namespace Arith_Funct7

// A layout without considering immediate values
struct command_layout {
    command_size_t opcode : 7;
    command_size_t rd     : 5;
    command_size_t funct3 : 3;
    command_size_t rs1    : 5;
    command_size_t rs2    : 5;
    command_size_t funct7 : 7;
};

static constexpr auto make_std(command_size_t cmd) -> command_layout {
    return std::bit_cast<command_layout>(cmd);
}

} // namespace __details

struct r_type : __details::crtp<r_type> {
    const command_size_t _op : 7;
    command_size_t rd        : 5;
    command_size_t funct3    : 3;
    command_size_t rs1       : 5;
    command_size_t rs2       : 5;
    command_size_t funct7    : 7;

    using Funct3 = __details::Arith_Funct3::Funct;
    using Funct7 = __details::Arith_Funct7::Funct;

    static constexpr command_size_t opcode = 0b0110011;

    explicit r_type() : _op(opcode), rd(0), funct3(0), rs1(0), rs2(0), funct7(0) {}
};


struct i_type : __details::crtp<i_type> {
    const command_size_t _op : 7;
    command_size_t rd        : 5;
    command_size_t funct3    : 3;
    command_size_t rs1       : 5;
    command_size_t imm       : 12;

    using Funct3 = __details::Arith_Funct3::Funct;
    using Funct7 = __details::Arith_Funct7::Funct;

    static constexpr command_size_t opcode = 0b0010011;

    explicit i_type() : _op(opcode), rd(0), funct3(0), rs1(0), imm(0) {}

    auto get_imm() const -> command_size_t { return __details::sign_extend<12>(imm); }

    void set_imm(command_size_t imm) { this->imm = imm; }
};


struct s_type : __details::crtp<s_type> {
    const command_size_t _op : 7;
    command_size_t imm_4_0   : 5;
    command_size_t funct3    : 3;
    command_size_t rs1       : 5;
    command_size_t rs2       : 5;
    command_size_t imm_11_5  : 7;

    enum Funct3 : command_size_t { SB = 0b000, SH = 0b001, SW = 0b010 };

    static constexpr command_size_t opcode = 0b0100011;

    explicit s_type() : _op(opcode), imm_4_0(0), funct3(0), rs1(0), rs2(0), imm_11_5(0) {}

    auto get_imm() const -> command_size_t {
        command_size_t imm = (imm_11_5 << 5) | imm_4_0;
        return __details::sign_extend<12>(imm);
    }

    void set_imm(command_size_t imm) {
        imm_4_0  = imm;
        imm_11_5 = imm >> 5;
    }
};


struct l_type : __details::crtp<l_type> {
    const command_size_t _op : 7;
    command_size_t rd        : 5;
    command_size_t funct3    : 3;
    command_size_t rs1       : 5;
    command_size_t imm       : 12;

    enum Funct3 : command_size_t { LB = 0b000, LH = 0b001, LW = 0b010, LBU = 0b100, LHU = 0b101 };

    static constexpr command_size_t opcode = 0b0000011;
    explicit l_type() : _op(opcode), rd(0), funct3(0), rs1(0), imm(0) {}

    auto get_imm() const -> command_size_t { return __details::sign_extend<12>(imm); }

    void set_imm(command_size_t imm) { this->imm = imm; }
};


struct b_type : __details::crtp<b_type> {
    const command_size_t _op : 7;
    command_size_t imm_11    : 1;
    command_size_t imm_4_1   : 4;
    command_size_t funct3    : 3;
    command_size_t rs1       : 5;
    command_size_t rs2       : 5;
    command_size_t imm_10_5  : 6;
    command_size_t imm_12    : 1;

    enum Funct3 : command_size_t {
        BEQ  = 0b000,
        BNE  = 0b001,
        BLT  = 0b100,
        BGE  = 0b101,
        BLTU = 0b110,
        BGEU = 0b111
    };

    static constexpr command_size_t opcode = 0b1100011;

    explicit b_type() :
        _op(opcode), imm_11(0), imm_4_1(0), funct3(0), rs1(0), rs2(0), imm_10_5(0), imm_12(0) {}

    auto get_imm() const -> command_size_t {
        command_size_t imm = (imm_12 << 12) | (imm_11 << 11) | (imm_10_5 << 5) | (imm_4_1 << 1);
        return __details::sign_extend<13>(imm);
    }

    void set_imm(command_size_t imm) {
        imm_11   = imm >> 11;
        imm_4_1  = imm >> 1;
        imm_10_5 = imm >> 5;
        imm_12   = imm >> 12;
    }
};

struct auipc : __details::crtp<auipc> {
    const command_size_t _op : 7;
    command_size_t rd        : 5;
    command_size_t imm       : 20;

    static constexpr command_size_t opcode = 0b0010111;

    explicit auipc() : _op(opcode), rd(0), imm(0) {}

    auto get_imm() const -> command_size_t { return imm << 12; }

    void set_imm(command_size_t imm) { this->imm = imm; }
};

struct lui : __details::crtp<lui> {
    const command_size_t _op : 7;
    command_size_t rd        : 5;
    command_size_t imm       : 20;

    static constexpr command_size_t opcode = 0b0110111;

    explicit lui() : _op(opcode), rd(0), imm(0) {}

    auto get_imm() const -> command_size_t { return imm << 12; }

    void set_imm(command_size_t imm) { this->imm = imm; }
};


struct jalr : __details::crtp<jalr> {
    const command_size_t _op    : 7;
    command_size_t rd           : 5;
    const command_size_t funct3 : 3;
    command_size_t rs1          : 5;
    command_size_t imm          : 12;

    static constexpr command_size_t opcode = 0b1100111;

    explicit constexpr jalr() : _op(opcode), rd(0), funct3(0), rs1(0), imm(0) {}

    auto get_imm() const -> command_size_t { return __details::sign_extend<12>(imm); }

    void set_imm(command_size_t imm) { this->imm = imm; }
};

struct jal : __details::crtp<jal> {
private:
    friend struct __details::crtp<jal>;
    const command_size_t _op : 7;

public:
    command_size_t rd        : 5;
    command_size_t imm_19_12 : 8;
    command_size_t imm_11    : 1;
    command_size_t imm_10_1  : 10;
    command_size_t imm_20    : 1;

    static constexpr command_size_t opcode = 0b1101111;

    explicit jal() : _op(opcode), rd(0), imm_19_12(0), imm_11(0), imm_10_1(0), imm_20(0) {}

    auto get_imm() const -> command_size_t {
        command_size_t imm = (imm_20 << 20) | (imm_19_12 << 12) | (imm_11 << 11) | (imm_10_1 << 1);
        return __details::sign_extend<21>(imm);
    }

    void set_imm(command_size_t imm) {
        imm_19_12 = imm >> 12;
        imm_11    = imm >> 11;
        imm_10_1  = imm >> 1;
        imm_20    = imm >> 20;
    }
};

static constexpr auto get_opcode(command_size_t cmd) {
    return __details::make_std(cmd).opcode;
}

static constexpr auto get_funct3(command_size_t cmd) {
    return __details::make_std(cmd).funct3;
}

static constexpr auto get_funct7(command_size_t cmd) {
    return __details::make_std(cmd).funct7;
}

static constexpr auto get_rd(command_size_t cmd) {
    return __details::make_std(cmd).rd;
}

namespace __details {

template <typename _Derived>
constexpr auto crtp<_Derived>::from_integer(command_size_t cmd) -> _Derived {
    return std::bit_cast<_Derived>(cmd);
}

} // namespace __details

} // namespace dark::command
