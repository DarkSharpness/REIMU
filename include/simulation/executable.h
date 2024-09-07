// Should only be included in interpreter/executable.cpp
#include "general.h"
#include "interpreter/device.h"
#include "interpreter/exception.h"
#include "interpreter/executable.h"
#include "interpreter/memory.h"
#include "interpreter/register.h"
#include "utility/error.h"

namespace dark {

struct Executable::MetaData::PackData {
    target_size_t &rd;
    target_size_t rs1;
    target_size_t rs2;
    target_size_t imm;
};

inline auto Executable::MetaData::parse(RegisterFile &rf) const -> PackData {
    auto &rd = rf[this->rd];
    auto rs1 = rf[this->rs1];
    auto rs2 = rf[this->rs2];
    auto imm = this->imm;
    return PackData{.rd = rd, .rs1 = rs1, .rs2 = rs2, .imm = imm};
}

} // namespace dark

namespace dark::interpreter {

namespace __details {

template <general::ArithOp op>
static void arith_impl(target_size_t &rd, target_size_t rs1, target_size_t rs2, Device &dev) {
    static_assert(sizeof(target_size_t) == 4);

    using i32 = std::int32_t;
    using u32 = std::uint32_t;

    constexpr auto i64 = [](u32 x) { return static_cast<std::int64_t>(i32(x)); };
    constexpr auto u64 = [](u32 x) { return static_cast<std::uint64_t>(x); };

    using enum general::ArithOp;
#define check_zero                                                                                 \
    rs2 == 0 ? throw FailToInterpret {                                                             \
        .error = Error::DivideByZero, .message = {}                                                \
    }

    switch (op) {
        case ADD:
            rd = rs1 + rs2;
            dev.counter.wArith++;
            return;
        case SUB:
            rd = rs1 - rs2;
            dev.counter.wArith++;
            return;
        case AND:
            rd = rs1 & rs2;
            dev.counter.wBitwise++;
            return;
        case OR:
            rd = rs1 | rs2;
            dev.counter.wBitwise++;
            return;
        case XOR:
            rd = rs1 ^ rs2;
            dev.counter.wBitwise++;
            return;
        case SLL:
            rd = rs1 << rs2;
            dev.counter.wShift++;
            return;
        case SRL:
            rd = u32(rs1) >> rs2;
            dev.counter.wShift++;
            return;
        case SRA:
            rd = i32(rs1) >> rs2;
            dev.counter.wShift++;
            return;
        case SLT:
            rd = i32(rs1) < i32(rs2);
            dev.counter.wCompare++;
            return;
        case SLTU:
            rd = u32(rs1) < u32(rs2);
            dev.counter.wCompare++;
            return;
        case MUL:
            rd = rs1 * rs2;
            dev.counter.wMultiply++;
            return;
        case MULH:
            rd = (i64(rs1) * i64(rs2)) >> 32;
            dev.counter.wMultiply++;
            return;
        case MULHSU:
            rd = (i64(rs1) * u64(rs2)) >> 32;
            dev.counter.wMultiply++;
            return;
        case MULHU:
            rd = (u64(rs1) * u64(rs2)) >> 32;
            dev.counter.wMultiply++;
            return;
        case DIV:
            rd = check_zero : i32(rs1) / i32(rs2);
            dev.counter.wDivide++;
            return;
        case DIVU:
            rd = check_zero : u32(rs1) / u32(rs2);
            dev.counter.wDivide++;
            return;
        case REM:
            rd = check_zero : i32(rs1) % i32(rs2);
            dev.counter.wDivide++;
            return;
        case REMU:
            rd = check_zero : u32(rs1) % u32(rs2);
            dev.counter.wDivide++;
            return;
        default: unreachable();
    }
#undef check_zero
}

} // namespace __details

namespace ArithReg {
template <general::ArithOp op>
static auto fn(Executable &exe, RegisterFile &rf, Memory &, Device &dev) {
    auto &&[rd, rs1, rs2, imm] = exe.get_meta().parse(rf);
    __details::arith_impl<op>(rd, rs1, rs2, dev);
    return exe.next();
}
} // namespace ArithReg

namespace ArithImm {
template <general::ArithOp op>
static auto fn(Executable &exe, RegisterFile &rf, Memory &, Device &dev) {
    auto &&[rd, rs1, rs2, imm] = exe.get_meta().parse(rf);
    __details::arith_impl<op>(rd, rs1, imm, dev);
    return exe.next();
}
} // namespace ArithImm

namespace LoadStore {
template <general::MemoryOp op>
static auto fn(Executable &exe, RegisterFile &rf, Memory &mem, Device &dev) {
    auto &&[rd, rs1, rs2, imm] = exe.get_meta().parse(rf);
    auto addr                  = rs1 + imm;

    using enum general::MemoryOp;
    switch (op) {
        case LB:
            rd = mem.load_i8(addr);
            dev.counter.wLoad++;
            dev.try_load(addr, 1);
            break;
        case LH:
            rd = mem.load_i16(addr);
            dev.counter.wLoad++;
            dev.try_load(addr, 2);
            break;
        case LW:
            rd = mem.load_i32(addr);
            dev.counter.wLoad++;
            dev.try_load(addr, 4);
            break;
        case LBU:
            rd = mem.load_u8(addr);
            dev.counter.wLoad++;
            dev.try_load(addr, 1);
            break;
        case LHU:
            rd = mem.load_u16(addr);
            dev.counter.wLoad++;
            dev.try_load(addr, 2);
            break;
        case SB:
            mem.store_u8(addr, rs2);
            dev.counter.wStore++;
            dev.try_store(addr, 1);
            break;
        case SH:
            mem.store_u16(addr, rs2);
            dev.counter.wStore++;
            dev.try_store(addr, 2);
            break;
        case SW:
            mem.store_u32(addr, rs2);
            dev.counter.wStore++;
            dev.try_store(addr, 4);
            break;
        default: unreachable();
    }

    return exe.next();
}
} // namespace LoadStore

namespace Branch {
template <general::BranchOp op>
static auto fn(Executable &exe, RegisterFile &rf, Memory &, Device &dev) {
    auto &&[rd, rs1, rs2, imm] = exe.get_meta().parse(rf);
    static_assert(sizeof(target_size_t) == 4);

    using i32 = std::int32_t;
    using u32 = std::uint32_t;

    using enum general::BranchOp;

    bool result{};
    switch (op) {
        case BEQ:
            result = (rs1 == rs2);
            dev.counter.wBranch++;
            break;
        case BNE:
            result = (rs1 != rs2);
            dev.counter.wBranch++;
            break;
        case BLT:
            result = (i32(rs1) < i32(rs2));
            dev.counter.wBranch++;
            break;
        case BGE:
            result = (i32(rs1) >= i32(rs2));
            dev.counter.wBranch++;
            break;
        case BLTU:
            result = (u32(rs1) < u32(rs2));
            dev.counter.wBranch++;
            break;
        case BGEU:
            result = (u32(rs1) >= u32(rs2));
            dev.counter.wBranch++;
            break;
        default: unreachable();
    }

    dev.predict(rf.get_pc(), result);
    if (result) {
        rf.set_pc(rf.get_pc() + imm);
        return exe.next(imm);
    } else {
        return exe.next();
    }
}
} // namespace Branch

namespace Jump {
[[maybe_unused]]
static auto fn(Executable &exe, RegisterFile &rf, Memory &, Device &dev) {
    auto &&[rd, rs1, rs2, imm] = exe.get_meta().parse(rf);

    rd = rf.get_pc() + 4;
    rf.set_pc(rf.get_pc() + imm);
    dev.counter.wJal++;

    return exe.next(imm);
}
} // namespace Jump

namespace Jalr {
[[maybe_unused]]
static auto fn(Executable &exe, RegisterFile &rf, Memory &, Device &dev) {
    auto &&[rd, rs1, rs2, imm] = exe.get_meta().parse(rf);

    auto target = (rs1 + imm) & ~1;
    auto offset = target - rf.get_pc();

    rd = rf.get_pc() + 4;
    rf.set_pc(target);
    dev.counter.wJalr++;

    return exe.next(offset);
}
} // namespace Jalr

namespace Lui {
[[maybe_unused]]
static auto fn(Executable &exe, RegisterFile &rf, Memory &, Device &dev) {
    auto &&[rd, rs1, rs2, imm] = exe.get_meta().parse(rf);
    rd                         = imm;
    dev.counter.wUpper++;
    return exe.next();
}
} // namespace Lui

namespace Auipc {
[[maybe_unused]]
static auto fn(Executable &exe, RegisterFile &rf, Memory &, Device &dev) {
    auto &&[rd, rs1, rs2, imm] = exe.get_meta().parse(rf);
    rd                         = rf.get_pc() + imm;
    dev.counter.wUpper++;
    return exe.next();
}
} // namespace Auipc

} // namespace dark::interpreter
