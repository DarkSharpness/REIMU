#pragma once
#include "assembly/forward.h"
#include "assembly/storage/immediate.h"
#include "assembly/storage/visitor.h"
#include "general.h"

namespace dark {

struct Command : Storage {
    using Storage::Storage;
};

struct ArithmeticReg final : Command {
    using Opcode = general::ArithOp;
    Opcode opcode;
    Register rd;
    Register rs1;
    Register rs2;

    explicit ArithmeticReg(LineInfo li, Opcode opcode, Register rd, Register rs1, Register rs2) :
        Command(li), opcode(opcode), rd(rd), rs1(rs1), rs2(rs2) {}

    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct ArithmeticImm final : Command {
    using Opcode = general::ArithOp;
    Opcode opcode;
    Register rd;
    Register rs1;
    Immediate imm;

    explicit ArithmeticImm(LineInfo li, Opcode opcode, Register rd, Register rs1, Immediate imm) :
        Command(li), opcode(opcode), rd(rd), rs1(rs1), imm(std::move(imm)) {}

    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct LoadStore final : Command {
    using Opcode = general::MemoryOp;
    Opcode opcode;
    Register rd;
    Register rs1;
    Immediate imm;

    explicit LoadStore(LineInfo li, Opcode opcode, Register rd, Register rs1, Immediate imm) :
        Command(li), opcode(opcode), rd(rd), rs1(rs1), imm(std::move(imm)) {}

    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }

    bool is_load() const {
        return opcode == Opcode::LB || opcode == Opcode::LH || opcode == Opcode::LW ||
               opcode == Opcode::LBU || opcode == Opcode::LHU;
    }
};

struct Branch final : Command {
    using Opcode = general::BranchOp;
    Opcode opcode;
    Register rs1;
    Register rs2;
    Immediate imm;

    explicit Branch(LineInfo li, Opcode opcode, Register rs1, Register rs2, Immediate imm) :
        Command(li), opcode(opcode), rs1(rs1), rs2(rs2), imm(std::move(imm)) {}

    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct JumpRelative final : Command {
    Register rd;
    Immediate imm;

    explicit JumpRelative(LineInfo li, Register rd, Immediate imm) :
        Command(li), rd(rd), imm(std::move(imm)) {}

    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct JumpRegister final : Command {
    Register rd;
    Register rs1;
    Immediate imm;

    explicit JumpRegister(LineInfo li, Register rd, Register rs1, Immediate imm) :
        Command(li), rd(rd), rs1(rs1), imm(std::move(imm)) {}

    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct CallFunction final : Command {
private:
    bool tail;

public:
    Immediate imm;

    explicit CallFunction(LineInfo li, bool tail, Immediate imm) :
        Command(li), tail(tail), imm(std::move(imm)) {}

    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }

    bool is_tail_call() const { return tail; }
};

struct LoadImmediate final : Command {
    Register rd;
    Immediate imm; // Immediate can be an address or a value

    explicit LoadImmediate(LineInfo li, Register rd, Immediate imm) :
        Command(li), rd(rd), imm(std::move(imm)) {}

    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct LoadUpperImmediate final : Command {
    Register rd;
    Immediate imm;

    explicit LoadUpperImmediate(LineInfo li, Register rd, Immediate imm) :
        Command(li), rd(rd), imm(std::move(imm)) {}

    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct AddUpperImmediatePC final : Command {
    Register rd;
    Immediate imm;

    explicit AddUpperImmediatePC(LineInfo li, Register rd, Immediate imm) :
        Command(li), rd(rd), imm(std::move(imm)) {}

    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

} // namespace dark
