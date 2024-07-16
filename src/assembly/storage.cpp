#include <utility.h>
#include <assembly/parser.h>
#include <assembly/storage.h>
#include <assembly/exception.h>
#include <fmtlib>

namespace dark {

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


void ArithmeticReg::debug(std::ostream &os) const {
    if (this->opcode == Opcode::ADD && this->rs2 == Register::zero) {
        os << std::format("    mv {}, {}", reg_to_sv(rd), reg_to_sv(rs1));
        return;
    }

    os << std::format("    {} {}, {}, {}",
        kArithMap[static_cast<std::size_t>(opcode)],
        reg_to_sv(rd), reg_to_sv(rs1), reg_to_sv(rs2));
}

void ArithmeticImm::debug(std::ostream &os) const {
    os << std::format("    {} {}, {}, {}",
        kImmMap[static_cast<std::size_t>(opcode)],
        reg_to_sv(rd), reg_to_sv(rs1), imm.to_string());
}

void LoadStore::debug(std::ostream &os) const {
    os << std::format("    {} {}, {}({})",
        kMemoryMap[static_cast<std::size_t>(opcode)],
        reg_to_sv(rd), imm.to_string(), reg_to_sv(rs1));
}

void Branch::debug(std::ostream &os) const {
    os << std::format("    {} {}, {}, {}",
        kBranchMap[static_cast<std::size_t>(opcode)],
        reg_to_sv(rs1), reg_to_sv(rs2), imm.to_string());
}

void JumpRelative::debug(std::ostream &os) const {
    if (this->rd == Register::zero)
        os << std::format("    j {}", imm.to_string());
    else
        os << std::format("    jal {}, {}", reg_to_sv(rd), imm.to_string());
}

void JumpRegister::debug(std::ostream &os) const {
    if (this->rd == Register::zero && this->imm.to_string() == "0") {
        if (this->rs1 == Register::ra)
            os << "    ret";
        else
            os << std::format("    jr {}", reg_to_sv(rs1));
    } else {
        os << std::format("    jalr {}, {}, {}",
            reg_to_sv(rd), reg_to_sv(rs1), imm.to_string());
    }
}

void LoadUpperImmediate::debug(std::ostream &os) const {
    os << std::format("    lui {}, {}", reg_to_sv(rd), imm.to_string());
}

void AddUpperImmediatePC::debug(std::ostream &os) const {
    os << std::format("    auipc {}, {}", reg_to_sv(rd), imm.to_string());
}

void LoadImmediate::debug(std::ostream &os) const {
    os << std::format("    li {}, {}", reg_to_sv(rd), imm.to_string());
}

void CallFunction::debug(std::ostream &os) const {
    const char *buff = this->tail ? "tail" : "call";
    os << std::format("    {} {}", buff, imm.to_string());
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
        default: unreachable();
    }
    os << data.to_string();
}

void ZeroBytes::debug(std::ostream &os) const {
    os << "    .zero " << count;
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

namespace dark {

Alignment::Alignment(std::size_t alignment) : alignment(alignment) {
    throw_if(!std::has_single_bit(alignment), "Invalid alignment: \"{}\"", alignment);
}

IntegerData::IntegerData(std::string_view data, Type type)
    : data(), type(type) {
    runtime_assert(Type::BYTE <= type && type <= Type::LONG);
    auto [value] = Parser {data} .match <Immediate> ();
    this->data = std::move(value);
}

ZeroBytes::ZeroBytes(std::size_t count) : count(count) {}

ASCIZ::ASCIZ(std::string str) : data(std::move(str)) {}

StrImmediate::StrImmediate(std::string_view data) : data(data) {
    for (char c : data) throw_if(!is_label_char(c), "Invalid label name: \"{}\"", data);
}

auto sv_to_reg(std::string_view view) -> Register {
    auto reg = sv_to_reg_nothrow(view);
    if (reg.has_value()) return *reg;
    throw FailToParse { std::format("Invalid register: \"{}\"", view) };
}

} // namespace dark
