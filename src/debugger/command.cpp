#include "riscv/command.h"
#include "declarations.h"
#include "fmtlib.h"
#include "riscv/register.h"
#include "simulation/debug.h"

namespace dark {

static auto default_format(command_size_t cmd) -> std::string {
    return fmt::format("{:#x}", cmd);
}

static auto pretty_r_type(command_size_t cmd) -> std::string {
    constexpr auto join = // A simple function to join two integers.
        [](std::uint32_t a, std::uint32_t b) -> std::uint32_t { return (a << 3) | b; };

    auto r_type = command::r_type::from_integer(cmd);
    auto rd     = int_to_reg(r_type.rd);
    auto rs1    = int_to_reg(r_type.rs1);
    auto rs2    = int_to_reg(r_type.rs2);

    auto suffix = fmt::format("{}, {}, {}", reg_to_sv(rd), reg_to_sv(rs1), reg_to_sv(rs2));

#define match_and_return(a, str)                                                                   \
    case join(command::r_type::Funct7::a, command::r_type::Funct3::a):                             \
        return fmt::format(str, suffix);

    switch (join(r_type.funct7, r_type.funct3)) {
        match_and_return(ADD, "add {}");
        match_and_return(SUB, "sub {}");
        match_and_return(SLL, "sll {}");
        match_and_return(SLT, "slt {}");
        match_and_return(SLTU, "sltu {}");
        match_and_return(XOR, "xor {}");
        match_and_return(SRL, "srl {}");
        match_and_return(SRA, "sra {}");
        match_and_return(OR, "or {}");
        match_and_return(AND, "and {}");
        match_and_return(MUL, "mul {}");
        match_and_return(MULH, "mulh {}");
        match_and_return(MULHSU, "mulhsu {}");
        match_and_return(MULHU, "mulhu {}");
        match_and_return(DIV, "div {}");
        match_and_return(DIVU, "divu {}");
        match_and_return(REM, "rem {}");
        match_and_return(REMU, "remu {}");

        default: return default_format(cmd);
    }
#undef match_and_return
}

static auto pretty_i_type(command_size_t cmd) -> std::string {
    auto i_type = command::i_type::from_integer(cmd);
    auto rd     = int_to_reg(i_type.rd);
    auto rs1    = int_to_reg(i_type.rs1);
    auto imm    = target_ssize_t(i_type.get_imm());
    if (i_type.funct3 == command::i_type::Funct3::ADD) {
        if (rs1 == Register::zero)
            return fmt::format("li {}, {}", reg_to_sv(rd), imm);
        if (imm == 0)
            return fmt::format("mv {}, {}", reg_to_sv(rd), reg_to_sv(rs1));
    }

    auto suffix = fmt::format("{}, {}, {}", reg_to_sv(rd), reg_to_sv(rs1), imm);

#define match_and_return(a, str)                                                                   \
    case command::i_type::Funct3::a: return fmt::format(str, suffix);

    switch (i_type.funct3) {
        match_and_return(ADD, "addi {}");
        match_and_return(SLT, "slti {}");
        match_and_return(SLTU, "sltiu {}");
        match_and_return(XOR, "xori {}");
        match_and_return(OR, "ori {}");
        match_and_return(AND, "andi {}");

        case command::i_type::Funct3::SLL:
            if (command::get_funct7(cmd) == command::i_type::Funct7::SLL)
                return fmt::format("slli {}", suffix);
            return default_format(cmd);

        case command::i_type::Funct3::SRL:
            if (command::get_funct7(cmd) == command::i_type::Funct7::SRL)
                return fmt::format("srli {}", suffix);
            if (command::get_funct7(cmd) == command::i_type::Funct7::SRA) {
                constexpr auto mask = sizeof(target_size_t) * 8 - 1;
                imm &= mask;
                return fmt::format("srai {}, {}, {}", reg_to_sv(rd), reg_to_sv(rs1), imm);
            }
            return default_format(cmd);

        default: return default_format(cmd);
    }
#undef match_and_return
}

static auto pretty_s_type(command_size_t cmd) -> std::string {
    auto s_type = command::s_type::from_integer(cmd);
    auto rs1    = int_to_reg(s_type.rs1);
    auto rs2    = int_to_reg(s_type.rs2);
    auto imm    = target_ssize_t(s_type.get_imm());
    auto suffix = fmt::format("{}, {}({})", reg_to_sv(rs2), imm, reg_to_sv(rs1));

#define match_and_return(a, str)                                                                   \
    case command::s_type::Funct3::a: return fmt::format(str, suffix);

    switch (s_type.funct3) {
        match_and_return(SW, "sw {}");
        match_and_return(SH, "sh {}");
        match_and_return(SB, "sb {}");
        default: return default_format(cmd);
    }
#undef match_and_return
}

static auto pretty_l_type(command_size_t cmd) -> std::string {
    auto l_type = command::l_type::from_integer(cmd);
    auto rd     = int_to_reg(l_type.rd);
    auto rs1    = int_to_reg(l_type.rs1);
    auto imm    = target_ssize_t(l_type.get_imm());
    auto suffix = fmt::format("{}, {}({})", reg_to_sv(rd), imm, reg_to_sv(rs1));

#define match_and_return(a, str)                                                                   \
    case command::l_type::Funct3::a: return fmt::format(str, suffix);

    switch (l_type.funct3) {
        match_and_return(LB, "lb {}");
        match_and_return(LH, "lh {}");
        match_and_return(LW, "lw {}");
        match_and_return(LBU, "lbu {}");
        match_and_return(LHU, "lhu {}");
        default: return default_format(cmd);
    }
#undef match_and_return
}

static auto pretty_b_type(command_size_t cmd, DebugManager &manager, target_size_t pc)
    -> std::string {
    auto b_type = command::b_type::from_integer(cmd);
    auto rs1    = int_to_reg(b_type.rs1);
    auto rs2    = int_to_reg(b_type.rs2);
    auto imm    = target_ssize_t(b_type.get_imm());
    auto suffix = fmt::format(
        "{}, {}, {}\t(a.k.a # $pc = {})", reg_to_sv(rs1), reg_to_sv(rs2), imm,
        manager.pretty_address(pc + imm)
    );

#define match_and_return(a, str)                                                                   \
    case command::b_type::Funct3::a: return fmt::format(str, suffix);

    switch (b_type.funct3) {
        match_and_return(BEQ, "beq {}");
        match_and_return(BNE, "bne {}");
        match_and_return(BLT, "blt {}");
        match_and_return(BGE, "bge {}");
        match_and_return(BLTU, "bltu {}");
        match_and_return(BGEU, "bgeu {}");
        default: return default_format(cmd);
    }
}

static auto pretty_jal(command_size_t cmd, DebugManager &manager, target_size_t pc) -> std::string {
    auto jal    = command::jal::from_integer(cmd);
    auto rd     = int_to_reg(jal.rd);
    auto imm    = target_ssize_t(jal.get_imm());
    auto suffix = manager.pretty_address(pc + imm);

    if (rd == Register::zero)
        return fmt::format("j {}\t(a.k.a # $pc = {})", imm, suffix);
    return fmt::format("jal {}, {}\t(a.k.a # $pc = {})", reg_to_sv(rd), imm, suffix);
}

static auto pretty_jalr(command_size_t cmd) -> std::string {
    auto jalr = command::jalr::from_integer(cmd);
    auto rd   = int_to_reg(jalr.rd);
    auto rs1  = int_to_reg(jalr.rs1);
    auto imm  = target_ssize_t(jalr.get_imm());

    if (rd == Register::zero && rs1 == Register::ra && imm == 0)
        return "ret";
    return fmt::format("jalr {}, {}, {}", reg_to_sv(rd), reg_to_sv(rs1), imm);
}

static auto pretty_lui(command_size_t cmd) -> std::string {
    auto lui = command::lui::from_integer(cmd);
    auto rd  = int_to_reg(lui.rd);
    auto imm = target_ssize_t(lui.get_imm());
    return fmt::format("lui {0:}, {1:}\t(a.k.a # ${0:} = {2:})", reg_to_sv(rd), imm >> 12, imm);
}

static auto pretty_auipc(command_size_t cmd) -> std::string {
    auto auipc  = command::auipc::from_integer(cmd);
    auto rd     = int_to_reg(auipc.rd);
    auto imm    = target_ssize_t(auipc.get_imm());
    auto suffix = fmt::format("{}, {}", reg_to_sv(rd), imm);

    return fmt::format("auipc {}", suffix);
}

auto DebugManager::pretty_command(command_size_t cmd, target_size_t pc) -> std::string {
    switch (command::get_opcode(cmd)) {
        case DebugManager::kEcall: // ecall: special type
            return "ecall";        // system call
        case command::r_type::opcode: return pretty_r_type(cmd);
        case command::i_type::opcode: return pretty_i_type(cmd);
        case command::s_type::opcode: return pretty_s_type(cmd);
        case command::l_type::opcode: return pretty_l_type(cmd);
        case command::b_type::opcode: return pretty_b_type(cmd, *this, pc);
        case command::jal::opcode:    return pretty_jal(cmd, *this, pc);
        case command::jalr::opcode:   return pretty_jalr(cmd);
        case command::lui::opcode:    return pretty_lui(cmd);
        case command::auipc::opcode:  return pretty_auipc(cmd);
        default:                      return default_format(cmd);
    }
}

auto DebugManager::pretty_address(target_size_t pc) -> std::string {
    if (pc >= this->stack_range.first) {
        auto top    = this->stack_range.second;
        auto offset = top - pc;
        return fmt::format("{:#x} <_stack_top - {}>", pc, offset);
    }

    auto [label, offset] = this->map.get(pc);
    return fmt::format("{:#x} <{} + {}>", pc, label, offset);
}

} // namespace dark
