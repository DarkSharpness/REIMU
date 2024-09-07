#include "assembly/storage/command.h"
#include "assembly/assembly.h"
#include "assembly/exception.h"
#include "assembly/frontend/match.h"
#include "assembly/immediate.h"
#include "riscv/register.h"
#include "utility/error.h"
#include "utility/hash.h"
#include <algorithm>

namespace dark {

using frontend::match;
using frontend::OffsetRegister;
using frontend::Token;
using frontend::TokenStream;

void Assembler::parse_command_new(std::string_view token, const Stream &rest) {
    using Aop = ArithmeticReg::Opcode;
    using Mop = LoadStore::Opcode;
    using Bop = Branch::Opcode;

    using Reg    = Register;
    using Imm    = Immediate;
    using OffReg = OffsetRegister;

    /**
     * =========================================================
     *                    Real instructions
     * =========================================================
     */

    constexpr auto __insert_arith_reg = [](Assembler *ptr, TokenStream rest, Aop opcode) {
        auto [rd, rs1, rs2] = match<Reg, Reg, Reg>(rest);
        ptr->push_new<ArithmeticReg>(opcode, rd, rs1, rs2);
    };
    constexpr auto __insert_arith_imm = [](Assembler *ptr, TokenStream rest, Aop opcode) {
        auto [rd, rs1, imm] = match<Reg, Reg, Imm>(rest);
        ptr->push_new<ArithmeticImm>(opcode, rd, rs1, imm);
    };
    static constexpr auto __load_store_label = [](Assembler *ptr, Mop opcode, Immediate &hi,
                                                  Immediate &lo, Register rd, Register rt) {
        // This is because only 32-bit int can be divided into lui + offset
        // for 64-bit, we plan to use a new command store-tmp instead
        static_assert(sizeof(target_size_t) == 4, "Only support 32-bit target");

        using enum RelImmediate::Operand;
        auto hi_imm = std::make_unique<RelImmediate>(std::move(hi), HI);
        auto lo_imm = std::make_unique<RelImmediate>(std::move(lo), LO);

        ptr->push_new<LoadUpperImmediate>(rt, Immediate{std::move(hi_imm)});
        ptr->push_new<LoadStore>(opcode, rd, rt, Immediate{std::move(lo_imm)});
    };
    constexpr auto __insert_load = [](Assembler *ptr, TokenStream rest, Mop opcode) {
        using PlaceHolder = Token::Placeholder;
        auto [rd, _]      = match<Reg, PlaceHolder>(rest);
        throw_if(rest.count_args() != 1); // Too many arguments.
        auto offreg = frontend::try_parse_offset_register(rest);
        if (offreg.has_value()) {
            auto &&[off, rs1] = *offreg;
            ptr->push_new<LoadStore>(opcode, rd, rs1, off);
        } else {
            /// TODO: implement tree copy to avoid matching twice
            auto hi = frontend::parse_immediate(rest);
            auto lo = frontend::parse_immediate(rest);

            __load_store_label(ptr, opcode, hi, lo, rd, rd);
        }
    };
    constexpr auto __insert_store = [](Assembler *ptr, TokenStream rest, Mop opcode) {
        if (rest.count_args() == 2) {
            auto [rs2, off_rs1] = match<Reg, OffReg>(rest);
            auto &&[off, rs1]   = off_rs1;
            ptr->push_new<LoadStore>(opcode, rs2, rs1, off);
        } else { // store rs2, label, rt
            auto bak = rest;

            /// TODO: implement tree copy to avoid matching twice
            auto [rs2, hi, rt] = match<Reg, Imm, Reg>(bak);
            auto [___, lo, __] = match<Reg, Imm, Reg>(rest);

            __load_store_label(ptr, opcode, hi, lo, rs2, rt);
        }
    };
    constexpr auto __insert_branch = [](Assembler *ptr, TokenStream rest, Bop opcode,
                                        bool swap = false) {
        auto [rs1, rs2, offset] = match<Reg, Reg, Imm>(rest);
        if (swap)
            std::swap(rs1, rs2);
        ptr->push_new<Branch>(opcode, rs1, rs2, offset);
    };
    constexpr auto __insert_jump = [](Assembler *ptr, TokenStream rest) {
        if (rest.count_args() == 1) {
            auto [offset] = match<Imm>(rest);
            using Register::ra;
            ptr->push_new<JumpRelative>(ra, offset);
        } else {
            auto [rd, offset] = match<Reg, Imm>(rest);
            ptr->push_new<JumpRelative>(rd, offset);
        }
    };
    constexpr auto __insert_jalr = [](Assembler *ptr, TokenStream rest) {
        if (rest.count_args() == 1) {
            auto [rs1] = match<Reg>(rest);
            using Register::ra;
            ptr->push_new<JumpRegister>(ra, rs1, Immediate(0));
        } else {
            auto [rd, off_rs1] = match<Reg, OffReg>(rest);
            auto &&[off, rs1]  = off_rs1;
            ptr->push_new<JumpRegister>(rd, rs1, off);
        }
    };
    constexpr auto __insert_lui = [](Assembler *ptr, TokenStream rest) {
        auto [rd, imm] = match<Reg, Imm>(rest);
        ptr->push_new<LoadUpperImmediate>(rd, imm);
    };
    constexpr auto __insert_auipc = [](Assembler *ptr, TokenStream rest) {
        auto [rd, imm] = match<Reg, Imm>(rest);
        ptr->push_new<AddUpperImmediatePC>(rd, imm);
    };

    /**
     * =========================================================
     *                    Pseudo instructions
     * =========================================================
     */

    constexpr auto __insert_mv = [](Assembler *ptr, TokenStream rest) {
        auto [rd, rs1] = match<Reg, Reg>(rest);
        ptr->push_new<ArithmeticImm>(Aop::ADD, rd, rs1, Immediate(0));
    };
    constexpr auto __insert_li = [](Assembler *ptr, TokenStream rest) {
        auto [rd, imm] = match<Reg, Imm>(rest);
        ptr->push_new<LoadImmediate>(rd, imm);
    };
    constexpr auto __insert_neg = [](Assembler *ptr, TokenStream rest) {
        auto [rd, rs1] = match<Reg, Reg>(rest);
        using Register::zero;
        ptr->push_new<ArithmeticReg>(Aop::SUB, rd, zero, rs1);
    };
    constexpr auto __insert_not = [](Assembler *ptr, TokenStream rest) {
        auto [rd, rs1] = match<Reg, Reg>(rest);
        ptr->push_new<ArithmeticImm>(Aop::XOR, rd, rs1, Immediate(-1));
    };
    constexpr auto __insert_seqz = [](Assembler *ptr, TokenStream rest) {
        auto [rd, rs1] = match<Reg, Reg>(rest);
        ptr->push_new<ArithmeticImm>(Aop::SLTU, rd, rs1, Immediate(1));
    };
    constexpr auto __insert_snez = [](Assembler *ptr, TokenStream rest) {
        auto [rd, rs1] = match<Reg, Reg>(rest);
        using Register::zero;
        ptr->push_new<ArithmeticReg>(Aop::SLTU, rd, zero, rs1);
    };
    constexpr auto __insert_sgtz = [](Assembler *ptr, TokenStream rest) {
        auto [rd, rs1] = match<Reg, Reg>(rest);
        using Register::zero;
        ptr->push_new<ArithmeticReg>(Aop::SLT, rd, zero, rs1);
    };
    constexpr auto __insert_sltz = [](Assembler *ptr, TokenStream rest) {
        auto [rd, rs1] = match<Reg, Reg>(rest);
        using Register::zero;
        ptr->push_new<ArithmeticReg>(Aop::SLT, rd, rs1, zero);
    };
    enum class Cmp_type { EQZ, NEZ, LTZ, GTZ, LEZ, GEZ };
    constexpr auto __insert_brz = [](Assembler *ptr, TokenStream rest, Cmp_type opcode) {
        auto [rs1, offset] = match<Reg, Imm>(rest);

#define try_match(cmp, op, ...)                                                                    \
    case cmp: ptr->push_new<Branch>(op, ##__VA_ARGS__, offset); return

        using Register::zero;

        switch (opcode) {
            try_match(Cmp_type::EQZ, Bop::BEQ, rs1, zero);
            try_match(Cmp_type::NEZ, Bop::BNE, rs1, zero);
            try_match(Cmp_type::LTZ, Bop::BLT, rs1, zero);
            try_match(Cmp_type::GTZ, Bop::BLT, zero, rs1);
            try_match(Cmp_type::LEZ, Bop::BGE, zero, rs1);
            try_match(Cmp_type::GEZ, Bop::BGE, rs1, zero);
            default: unreachable();
        }
#undef try_match
    };
    constexpr auto __insert_call = [](Assembler *ptr, TokenStream rest, bool is_tail) {
        auto [offset] = match<Imm>(rest);
        ptr->push_new<CallFunction>(is_tail, offset);
    };
    constexpr auto __insert_j = [](Assembler *ptr, TokenStream rest) {
        auto [offset] = match<Imm>(rest);
        using Register::zero;
        ptr->push_new<JumpRelative>(zero, offset);
    };
    constexpr auto __insert_jr = [](Assembler *ptr, TokenStream rest) {
        auto [rs1] = match<Reg>(rest);
        using Register::zero;
        ptr->push_new<JumpRegister>(zero, rs1, Immediate(0));
    };
    constexpr auto __insert_ret = [](Assembler *ptr, TokenStream rest) {
        static_cast<void>(match<>(rest));
        using Register::zero, Register::ra;
        ptr->push_new<JumpRegister>(zero, ra, Immediate(0));
    };
    constexpr auto __insert_lla = [](Assembler *ptr, TokenStream rest) {
        auto [rd, offset] = match<Reg, Imm>(rest);
        ptr->push_new<LoadImmediate>(rd, offset);
    };
    constexpr auto __insert_nop = [](Assembler *ptr, TokenStream rest) {
        static_cast<void>(match<>(rest));
        using Register::zero;
        ptr->push_new<ArithmeticImm>(Aop::ADD, zero, zero, Immediate(0));
    };

    using hash::switch_hash_impl;

#define match_or_break(str, expr, ...)                                                             \
    case switch_hash_impl(str):                                                                    \
        if (token == str) {                                                                        \
            return expr(this, rest, ##__VA_ARGS__);                                                \
        }                                                                                          \
        break

    switch (switch_hash_impl(token)) {
        match_or_break("add", __insert_arith_reg, Aop::ADD);
        match_or_break("sub", __insert_arith_reg, Aop::SUB);
        match_or_break("and", __insert_arith_reg, Aop::AND);
        match_or_break("or", __insert_arith_reg, Aop::OR);
        match_or_break("xor", __insert_arith_reg, Aop::XOR);
        match_or_break("sll", __insert_arith_reg, Aop::SLL);
        match_or_break("srl", __insert_arith_reg, Aop::SRL);
        match_or_break("sra", __insert_arith_reg, Aop::SRA);
        match_or_break("slt", __insert_arith_reg, Aop::SLT);
        match_or_break("sltu", __insert_arith_reg, Aop::SLTU);

        match_or_break("mul", __insert_arith_reg, Aop::MUL);
        match_or_break("mulh", __insert_arith_reg, Aop::MULH);
        match_or_break("mulhu", __insert_arith_reg, Aop::MULHU);
        match_or_break("mulhsu", __insert_arith_reg, Aop::MULHSU);
        match_or_break("div", __insert_arith_reg, Aop::DIV);
        match_or_break("divu", __insert_arith_reg, Aop::DIVU);
        match_or_break("rem", __insert_arith_reg, Aop::REM);
        match_or_break("remu", __insert_arith_reg, Aop::REMU);

        match_or_break("addi", __insert_arith_imm, Aop::ADD);
        match_or_break("andi", __insert_arith_imm, Aop::AND);
        match_or_break("ori", __insert_arith_imm, Aop::OR);
        match_or_break("xori", __insert_arith_imm, Aop::XOR);
        match_or_break("slli", __insert_arith_imm, Aop::SLL);
        match_or_break("srli", __insert_arith_imm, Aop::SRL);
        match_or_break("srai", __insert_arith_imm, Aop::SRA);
        match_or_break("slti", __insert_arith_imm, Aop::SLT);
        match_or_break("sltiu", __insert_arith_imm, Aop::SLTU);

        match_or_break("lb", __insert_load, Mop::LB);
        match_or_break("lh", __insert_load, Mop::LH);
        match_or_break("lw", __insert_load, Mop::LW);
        match_or_break("lbu", __insert_load, Mop::LBU);
        match_or_break("lhu", __insert_load, Mop::LHU);
        match_or_break("sb", __insert_store, Mop::SB);
        match_or_break("sh", __insert_store, Mop::SH);
        match_or_break("sw", __insert_store, Mop::SW);

        match_or_break("beq", __insert_branch, Bop::BEQ);
        match_or_break("bne", __insert_branch, Bop::BNE);
        match_or_break("blt", __insert_branch, Bop::BLT);
        match_or_break("bge", __insert_branch, Bop::BGE);
        match_or_break("bltu", __insert_branch, Bop::BLTU);
        match_or_break("bgeu", __insert_branch, Bop::BGEU);

        match_or_break("jal", __insert_jump);
        match_or_break("jalr", __insert_jalr);

        match_or_break("lui", __insert_lui);
        match_or_break("auipc", __insert_auipc);

        match_or_break("mv", __insert_mv);
        match_or_break("li", __insert_li);

        match_or_break("neg", __insert_neg);
        match_or_break("not", __insert_not);

        match_or_break("seqz", __insert_seqz);
        match_or_break("snez", __insert_snez);
        match_or_break("sgtz", __insert_sgtz);
        match_or_break("sltz", __insert_sltz);

        match_or_break("beqz", __insert_brz, Cmp_type::EQZ);
        match_or_break("bnez", __insert_brz, Cmp_type::NEZ);
        match_or_break("bltz", __insert_brz, Cmp_type::LTZ);
        match_or_break("bgtz", __insert_brz, Cmp_type::GTZ);
        match_or_break("blez", __insert_brz, Cmp_type::LEZ);
        match_or_break("bgez", __insert_brz, Cmp_type::GEZ);

        match_or_break("ble", __insert_branch, Bop::BGE, /* swap = */ true);
        match_or_break("bleu", __insert_branch, Bop::BGEU, /* swap = */ true);
        match_or_break("bgt", __insert_branch, Bop::BLT, /* swap = */ true);
        match_or_break("bgtu", __insert_branch, Bop::BLTU, /* swap = */ true);

        match_or_break("call", __insert_call, /* tail_call = */ false);
        match_or_break("tail", __insert_call, /* tail_call = */ true);

        match_or_break("j", __insert_j);
        match_or_break("jr", __insert_jr);
        match_or_break("ret", __insert_ret);

        match_or_break("la", __insert_lla);
        match_or_break("lla", __insert_lla);

        match_or_break("nop", __insert_nop);

        default: break;
    }
    throw FailToParse{fmt::format("Unknown command: \"{}\"", token)};
}

} // namespace dark
