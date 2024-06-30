#pragma once
#include <declarations.h>
#include <riscv/register.h>
#include <utility/ustring.h>
#include <vector>

// Immediate

namespace dark {

struct ImmediateBase { virtual ~ImmediateBase() = default; };

struct Immediate {
    std::unique_ptr <ImmediateBase> data;
    explicit Immediate(std::unique_ptr <ImmediateBase> data) : data(std::move(data)) {}
    explicit Immediate(std::string_view view);
    std::string to_string() const;
};

struct IntImmediate : ImmediateBase {
    target_size_t data;
    explicit IntImmediate(target_size_t data) : data(data) {}
};

struct StrImmediate : ImmediateBase  {
    unique_string data;
    explicit StrImmediate(std::string_view data);
};

struct RelImmediate : ImmediateBase {
    Immediate imm;
    enum class Operand {
       HI, LO, PCREL_HI, PCREL_LO
    } operand;
    explicit RelImmediate(std::string_view imm, Operand op) : imm(imm), operand(op) {}
};

struct TreeImmediate : ImmediateBase {
    enum class Operator { ADD, SUB, END };
    struct Pair {
        Immediate imm; Operator op;
    };
    std::vector <Pair> data;
    explicit TreeImmediate(std::vector <Pair> data) : data(std::move(data)) {}
};

} // namespace dark

// Storage Visitor

namespace dark {

struct ArithmeticReg;
struct ArithmeticImm;
struct LoadStore;
struct Branch;
struct JumpRelative;
struct JumpRegister;
struct CallFunction;
struct LoadImmediate;
struct LoadUpperImmediate;
struct AddUpperImmediatePC;

struct Alignment;
struct IntegerData;
struct ZeroBytes;
struct ASCIZ;

struct StorageVisitor {
    virtual ~StorageVisitor() = default;
    void visit(Storage &storage) { return storage.accept(*this); }
    virtual void visitStorage(ArithmeticReg &storage) = 0;
    virtual void visitStorage(ArithmeticImm &storage) = 0;
    virtual void visitStorage(LoadStore &storage) = 0;
    virtual void visitStorage(Branch &storage) = 0;
    virtual void visitStorage(JumpRelative &storage) = 0;
    virtual void visitStorage(JumpRegister &storage) = 0;
    virtual void visitStorage(CallFunction &storage) = 0;
    virtual void visitStorage(LoadImmediate &storage) = 0;
    virtual void visitStorage(LoadUpperImmediate &storage) = 0;
    virtual void visitStorage(AddUpperImmediatePC &storage) = 0;
    virtual void visitStorage(Alignment &storage) = 0;
    virtual void visitStorage(IntegerData &storage) = 0;
    virtual void visitStorage(ZeroBytes &storage) = 0;
    virtual void visitStorage(ASCIZ &storage) = 0;
};

} // namespace dark

// Storage Command

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

struct Command : Storage {};

struct ArithmeticReg final : Command {
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
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct ArithmeticImm final : Command {
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

    explicit ArithmeticImm
        (Opcode opcode, Register rd, Register rs1, Immediate imm) :
        opcode(opcode),
        rd(rd),
        rs1(rs1),
        imm(std::move(imm)) {}

    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct LoadStore final : Command {
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
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }

    bool is_load() const {
        return opcode == Opcode::LB || opcode == Opcode::LH || opcode == Opcode::LW ||
               opcode == Opcode::LBU || opcode == Opcode::LHU;
    }
};

struct Branch final : Command {
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
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct JumpRelative final : Command {
    Register rd;
    Immediate imm;

    explicit JumpRelative(std::string_view rd, std::string_view imm) :
        rd(sv_to_reg(rd)), imm(imm) {}

    explicit JumpRelative(Register rd, Immediate imm) :
        rd(rd), imm(std::move(imm)) {}

    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct JumpRegister final : Command {
    Register rd;
    Register rs1;
    Immediate imm;

    explicit JumpRegister(std::string_view rd, std::string_view rs1, std::string_view imm) :
        rd(sv_to_reg(rd)), rs1(sv_to_reg(rs1)), imm(imm) {}

    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct CallFunction final : Command {
  private:
    bool tail;
  public:
    Immediate imm;

    explicit CallFunction(bool is_tail, std::string_view imm) :
        tail(is_tail), imm(imm) {}

    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }

    bool is_tail_call() const { return tail; }
};

struct LoadImmediate final : Command {
    Register rd;
    Immediate imm; // Immediate can be an address or a value

    explicit LoadImmediate(std::string_view rd, std::string_view imm) :
        rd(sv_to_reg(rd)), imm(imm) {}

    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct LoadUpperImmediate final : Command {
    Register rd;
    Immediate imm;

    explicit LoadUpperImmediate(std::string_view rd, std::string_view imm) :
        rd(sv_to_reg(rd)), imm(imm) {}

    explicit LoadUpperImmediate(Register rd, Immediate imm) :
        rd(rd), imm(std::move(imm)) {}

    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct AddUpperImmediatePC final : Command {
    Register rd;
    Immediate imm;

    explicit AddUpperImmediatePC(std::string_view rd, std::string_view imm) :
        rd(sv_to_reg(rd)), imm(imm) {}

    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

} // namespace dark

// Storage Data

namespace dark {

struct RealData : Storage {};

struct Alignment final : RealData {
    std::size_t alignment;
    explicit Alignment(std::size_t alignment);
    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct IntegerData final : RealData {
    Immediate data;
    enum class Type {
        BYTE = 0, SHORT = 1, LONG = 2
    } type;
    explicit IntegerData(std::string_view data, Type type);
    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct ZeroBytes final : RealData {
    std::size_t count;
    explicit ZeroBytes(std::size_t count);
    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct ASCIZ final : RealData {
    std::string data;
    explicit ASCIZ(std::string str);
    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

bool is_label_char(char c);

} // namespace dark
