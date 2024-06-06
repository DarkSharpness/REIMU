#pragma once
#include "assembly.h"
#include "register.h"

namespace dark {

struct Command : Assembler::Storage {
    ~Command() override = default;
};

namespace __details {

enum class ArithOpcode : std::uint8_t {
    // Normal arithmetic
    ADD, SUB, AND, OR, XOR, SLL, SRL, SRA, SLT, SLTU,
    // Multiplication and division extension
    MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU
};

enum class MemoryOpcode : std::uint8_t {
    LB, LH, LW, LBU, LHU, SB, SH, SW
};

enum class BranchOpcode : std::uint8_t {
    BEQ, BNE, BLT, BGE, BLTU, BGEU
};

} // namespace __details

struct Immediate {
    std::unique_ptr <char[]> value;
    std::size_t size;

    Immediate(std::string_view view) :
        value(std::make_unique<char[]>(view.size())), size(view.size()) 
    { for (std::size_t i = 0; i < size; ++i) value[i] = view[i]; }

    std::string_view to_string() const { return std::string_view(value.get(), size); }
};

struct ArithmeticReg : Command {
    using Opcode = __details::ArithOpcode;
    Opcode opcode;
    Register rd;
    Register rs1;
    Register rs2;

    explicit ArithmeticReg
        (Opcode opcode, std::string_view rd, std::string_view rs1, std::string_view rs2) :
        opcode(opcode),
        rd(sv_to_reg(rd)),
        rs1(sv_to_reg(rs1)),
        rs2(sv_to_reg(rs2)) {}

    virtual void debug(std::ostream &os) const override;
};

struct ArithmeticImm : Command {
    using Opcode = __details::ArithOpcode;
    Opcode opcode;
    Register rd;
    Register rs1;
    Immediate imm;

    explicit ArithmeticImm
        (Opcode opcode, std::string_view rd, std::string_view rs1, std::string_view imm) :
        opcode(opcode),
        rd(sv_to_reg(rd)),
        rs1(sv_to_reg(rs1)),
        imm(imm) {}

    virtual void debug(std::ostream &os) const override;
};

struct LoadStore : Command {
    using Opcode = __details::MemoryOpcode;
    Opcode opcode;
    Register rd;
    Register rs1;
    Immediate imm;

    explicit LoadStore
        (Opcode opcode, std::string_view rd, std::string_view rs1, std::string_view imm) :
        opcode(opcode),
        rd(sv_to_reg(rd)),
        rs1(sv_to_reg(rs1)),
        imm(imm) {}

    virtual void debug(std::ostream &os) const override;
};

struct Branch : Command {
    using Opcode = __details::BranchOpcode;
    Opcode opcode;
    Register rs1;
    Register rs2;
    Immediate imm;

    explicit Branch
        (Opcode opcode, std::string_view rs1, std::string_view rs2, std::string_view imm) :
        opcode(opcode),
        rs1(sv_to_reg(rs1)),
        rs2(sv_to_reg(rs2)),
        imm(imm) {}

    virtual void debug(std::ostream &os) const override;
};


struct JumpRelative : Command {
    Register rd;
    Immediate imm;

    explicit JumpRelative(std::string_view rd, std::string_view imm) :
        rd(sv_to_reg(rd)),
        imm(imm) {}

    virtual void debug(std::ostream &os) const override;
};

struct JumpRegister : Command {
    Register rd;
    Register rs1;
    Immediate imm;

    explicit JumpRegister(std::string_view rd, std::string_view rs1, std::string_view imm) :
        rd(sv_to_reg(rd)),
        rs1(sv_to_reg(rs1)),
        imm(imm) {}

    virtual void debug(std::ostream &os) const override;
};

struct CallFunction : Command {
    bool tail;
    Immediate imm;

    explicit CallFunction(bool is_tail, std::string_view imm) :
        tail(is_tail),
        imm(imm) {}

    virtual void debug(std::ostream &os) const override;
};

struct LoadImmediate : Command {
    Register rd;
    Immediate imm; // Immediate can be an address or a value

    explicit LoadImmediate(std::string_view rd, std::string_view imm) :
        rd(sv_to_reg(rd)),
        imm(imm) {}

    virtual void debug(std::ostream &os) const override;
};

struct LoadUpperImmediate : Command {
    Register rd;
    Immediate imm;

    explicit LoadUpperImmediate(std::string_view rd, std::string_view imm) :
        rd(sv_to_reg(rd)),
        imm(imm) {}

    virtual void debug(std::ostream &os) const override;
};

struct AddUpperImmediatePC : Command {
    Register rd;
    Immediate imm;

    explicit AddUpperImmediatePC(std::string_view rd, std::string_view imm) :
        rd(sv_to_reg(rd)),
        imm(imm) {}

    virtual void debug(std::ostream &os) const override;
};

} // namespace dark
