#include <storage.h>
#include <format>

namespace dark::__details {

static constexpr std::string_view kArithMap[] = {
    "add", "sub", "and", "or", "xor", "sll", "srl", "sra", "slt", "sltu",
    "mul", "mulh", "mulhsu", "mulhu", "div", "divu", "rem", "remu"
};

static constexpr std::string_view kImmMap[] = {
    "addi", {}, "andi", "ori", "xori", "slli", "srli", "srai", "slti", "sltiu",
};

static constexpr std::string_view kMemoryMap[] = {
    "lb", "lh", "lw", "lbu", "lhu", "sb", "sh", "sw"
};

static constexpr std::string_view kBranchMap[] = {
    "beq", "bne", "blt", "bge", "bltu", "bgeu"
};

static constexpr std::string_view kRegisterMap[] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3",
    "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4",
    "t5", "t6"
};

} // namespace dark::__details

namespace dark {

std::string_view reg_to_sv(Register reg) {
    using namespace __details;
    std::size_t which = static_cast<std::size_t>(reg);
    return kRegisterMap[which];
}

void ArithmeticReg::debug(std::ostream &os) const {
    using namespace __details;
    if (this->opcode == Opcode::ADD && this->rs2 == Register::zero) {
        os << std::format("    mv {}, {}",
            kRegisterMap[static_cast<std::size_t>(rd)],
            kRegisterMap[static_cast<std::size_t>(rs1)]);
        return;
    }

    os << std::format("    {} {}, {}, {}",
        kArithMap[static_cast<std::size_t>(opcode)],
        kRegisterMap[static_cast<std::size_t>(rd)],
        kRegisterMap[static_cast<std::size_t>(rs1)],
        kRegisterMap[static_cast<std::size_t>(rs2)]);
}

void ArithmeticImm::debug(std::ostream &os) const {
    using namespace __details;
    os << std::format("    {} {}, {}, {}",
        kImmMap[static_cast<std::size_t>(opcode)],
        kRegisterMap[static_cast<std::size_t>(rd)],
        kRegisterMap[static_cast<std::size_t>(rs1)],
        imm.to_string());
}

void LoadStore::debug(std::ostream &os) const {
    using namespace __details;
    os << std::format("    {} {}, {}({})",
        kMemoryMap[static_cast<std::size_t>(opcode)],
        kRegisterMap[static_cast<std::size_t>(rd)],
        imm.to_string(),
        kRegisterMap[static_cast<std::size_t>(rs1)]);
}

void Branch::debug(std::ostream &os) const {
    using namespace __details;
    os << std::format("    {} {}, {}, {}",
        kBranchMap[static_cast<std::size_t>(opcode)],
        kRegisterMap[static_cast<std::size_t>(rs1)],
        kRegisterMap[static_cast<std::size_t>(rs2)],
        imm.to_string());
}

void JumpRelative::debug(std::ostream &os) const {
    using namespace __details;
    if (this->rd == Register::zero)
        os << std::format("    j {}",
            imm.to_string());
    else
        os << std::format("    jal {}, {}",
            kRegisterMap[static_cast<std::size_t>(rd)], imm.to_string());
}

void JumpRegister::debug(std::ostream &os) const {
    using namespace __details;
    if (this->rd == Register::zero && this->imm.to_string() == "0") {
        if (this->rs1 == Register::ra)
            os << "    ret";
        else
            os << std::format("    jr {}",
                kRegisterMap[static_cast<std::size_t>(rs1)]);
    } else {
        os << std::format("    jalr {}, {}, {}",
            kRegisterMap[static_cast<std::size_t>(rd)],
            kRegisterMap[static_cast<std::size_t>(rs1)],
            imm.to_string());
    }
}

void LoadUpperImmediate::debug(std::ostream &os) const {
    os << std::format("    lui {}, {}",
        __details::kRegisterMap[static_cast<std::size_t>(rd)], imm.to_string());
}

void AddUpperImmediatePC::debug(std::ostream &os) const {
    os << std::format("    auipc {}, {}",
        __details::kRegisterMap[static_cast<std::size_t>(rd)], imm.to_string());
}

void LoadImmediate::debug(std::ostream &os) const {
    os << std::format("    li {}, {}",
        __details::kRegisterMap[static_cast<std::size_t>(rd)], imm.to_string());
}

void CallFunction::debug(std::ostream &os) const {
    const char *buff = this->tail ? "tail" : "call";
    os << std::format("    {} {}",
        buff, imm.to_string());
}

void Alignment::debug(std::ostream &os) const {
    os << "    .align " << std::countr_zero(alignment);
}

void IntegerData::debug(std::ostream &os) const {
    os << "    .";
    switch (type) {
        case Type::BYTE:  os << "byte "; break;
        case Type::SHORT: os << "half "; break;
        case Type::LONG:  os << "word "; break;
    }
    os << data.to_string();
}

void ZeroBytes::debug(std::ostream &os) const {
    os << "    .zero " << size;
}

void ASCIZ::debug(std::ostream &os) const {
    os << "    .asciz \"";
    for (char c : data) {
        switch (c) {
            case '\n': os << "\\n"; break;
            case '\t': os << "\\t"; break;
            case '\r': os << "\\r"; break;
            case '\0': os << "\\0"; break;
            case '\\': os << "\\\\"; break;
            case '\"': os << "\\\""; break;
            default: os << c;
        }
    }
    os << "\"";
}

} // namespace dark
