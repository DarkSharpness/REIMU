#pragma once
#include <declarations.h>
#include <register.h>
#include <ustring.h>

namespace dark {

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

struct ImmediateBase { virtual ~ImmediateBase() = default; };

struct RawImmediate : ImmediateBase {
    unique_string data;
    explicit RawImmediate(std::string_view view);
    std::string_view to_string() const;
};

struct Immediate {
    std::unique_ptr <ImmediateBase> data;
    explicit Immediate(std::string_view view);
    std::string_view to_string() const;
};

struct Command : Storage {
    ~Command() override = default;
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

    void debug(std::ostream &os) const override;
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

    void debug(std::ostream &os) const override;
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

    void debug(std::ostream &os) const override;
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

    void debug(std::ostream &os) const override;
};

struct JumpRelative : Command {
    Register rd;
    Immediate imm;

    explicit JumpRelative(std::string_view rd, std::string_view imm) :
        rd(sv_to_reg(rd)),
        imm(imm) {}

    void debug(std::ostream &os) const override;
};

struct JumpRegister : Command {
    Register rd;
    Register rs1;
    Immediate imm;

    explicit JumpRegister(std::string_view rd, std::string_view rs1, std::string_view imm) :
        rd(sv_to_reg(rd)),
        rs1(sv_to_reg(rs1)),
        imm(imm) {}

    void debug(std::ostream &os) const override;
};

struct CallFunction : Command {
    bool tail;
    Immediate imm;

    explicit CallFunction(bool is_tail, std::string_view imm) :
        tail(is_tail),
        imm(imm) {}

    void debug(std::ostream &os) const override;
};

struct LoadImmediate : Command {
    Register rd;
    Immediate imm; // Immediate can be an address or a value

    explicit LoadImmediate(std::string_view rd, std::string_view imm) :
        rd(sv_to_reg(rd)),
        imm(imm) {}

    void debug(std::ostream &os) const override;
};

struct LoadUpperImmediate : Command {
    Register rd;
    Immediate imm;

    explicit LoadUpperImmediate(std::string_view rd, std::string_view imm) :
        rd(sv_to_reg(rd)),
        imm(imm) {}

    void debug(std::ostream &os) const override;
};

struct AddUpperImmediatePC : Command {
    Register rd;
    Immediate imm;

    explicit AddUpperImmediatePC(std::string_view rd, std::string_view imm) :
        rd(sv_to_reg(rd)),
        imm(imm) {}

    void debug(std::ostream &os) const override;
};

} // namespace dark

namespace dark {

struct RealData : Storage {};

struct Alignment : RealData {
    std::size_t alignment;
    explicit Alignment(std::size_t alignment);
    void debug(std::ostream &os) const override;
};

struct IntegerData : RealData {
    Immediate data;
    enum class Type {
        BYTE = 0, SHORT = 1, LONG = 2
    } type;
    explicit IntegerData(std::string_view data, Type type);
    void debug(std::ostream &os) const override;
};

struct ZeroBytes : RealData {
    std::size_t size;
    explicit ZeroBytes(std::size_t size);
    void debug(std::ostream &os) const override;
};

struct ASCIZ : RealData {
    std::string data;
    explicit ASCIZ(std::string str);
    void debug(std::ostream &os) const override;
};


} // namespace dark
