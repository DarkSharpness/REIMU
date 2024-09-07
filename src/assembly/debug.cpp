#include "assembly/immediate.h"
#include "assembly/storage/command.h"
#include "assembly/storage/static.h"
#include "fmtlib.h"
#include "utility/error.h"

namespace dark {

static constexpr std::string_view kArithMap[] = {"add",    "sub",   "and", "or",   "xor", "sll",
                                                 "srl",    "sra",   "slt", "sltu", "mul", "mulh",
                                                 "mulhsu", "mulhu", "div", "divu", "rem", "remu"};

static constexpr std::string_view kImmMap[] = {
    "addi", {}, "andi", "ori", "xori", "slli", "srli", "srai", "slti", "sltiu",
};

static constexpr std::string_view kMemoryMap[] = {"lb", "lh", "lw", "lbu", "lhu", "sb", "sh", "sw"};

static constexpr std::string_view kBranchMap[] = {"beq", "bne", "blt", "bge", "bltu", "bgeu"};


void ArithmeticReg::debug(std::ostream &os) const {
    if (this->opcode == Opcode::ADD && this->rs2 == Register::zero) {
        os << fmt::format("    mv {}, {}", reg_to_sv(rd), reg_to_sv(rs1));
        return;
    }

    os << fmt::format(
        "    {} {}, {}, {}", kArithMap[static_cast<std::size_t>(opcode)], reg_to_sv(rd),
        reg_to_sv(rs1), reg_to_sv(rs2)
    );
}

void ArithmeticImm::debug(std::ostream &os) const {
    os << fmt::format(
        "    {} {}, {}, {}", kImmMap[static_cast<std::size_t>(opcode)], reg_to_sv(rd),
        reg_to_sv(rs1), imm.to_string()
    );
}

void LoadStore::debug(std::ostream &os) const {
    os << fmt::format(
        "    {} {}, {}({})", kMemoryMap[static_cast<std::size_t>(opcode)], reg_to_sv(rd),
        imm.to_string(), reg_to_sv(rs1)
    );
}

void Branch::debug(std::ostream &os) const {
    os << fmt::format(
        "    {} {}, {}, {}", kBranchMap[static_cast<std::size_t>(opcode)], reg_to_sv(rs1),
        reg_to_sv(rs2), imm.to_string()
    );
}

void JumpRelative::debug(std::ostream &os) const {
    if (this->rd == Register::zero)
        os << fmt::format("    j {}", imm.to_string());
    else
        os << fmt::format("    jal {}, {}", reg_to_sv(rd), imm.to_string());
}

void JumpRegister::debug(std::ostream &os) const {
    if (this->rd == Register::zero && this->imm.to_string() == "0") {
        if (this->rs1 == Register::ra)
            os << "    ret";
        else
            os << fmt::format("    jr {}", reg_to_sv(rs1));
    } else {
        os << fmt::format("    jalr {}, {}, {}", reg_to_sv(rd), reg_to_sv(rs1), imm.to_string());
    }
}

void LoadUpperImmediate::debug(std::ostream &os) const {
    os << fmt::format("    lui {}, {}", reg_to_sv(rd), imm.to_string());
}

void AddUpperImmediatePC::debug(std::ostream &os) const {
    os << fmt::format("    auipc {}, {}", reg_to_sv(rd), imm.to_string());
}

void LoadImmediate::debug(std::ostream &os) const {
    os << fmt::format("    li {}, {}", reg_to_sv(rd), imm.to_string());
}

void CallFunction::debug(std::ostream &os) const {
    const char *buff = this->tail ? "tail" : "call";
    os << fmt::format("    {} {}", buff, imm.to_string());
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
        default:          unreachable();
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
            default:   os << c;
        }
    }
    os << "\"";
}

} // namespace dark

namespace dark {

static auto imm_to_string(ImmediateBase *imm) -> std::string {
    if (auto ptr = dynamic_cast<IntImmediate *>(imm)) {
        return std::to_string(static_cast<target_ssize_t>(ptr->data));
    } else if (auto ptr = dynamic_cast<StrImmediate *>(imm)) {
        return std::string(ptr->data.to_sv());
    } else if (auto ptr = dynamic_cast<RelImmediate *>(imm)) {
        std::string_view op;
        switch (ptr->operand) {
            using enum RelImmediate::Operand;
            case HI:       op = "hi"; break;
            case LO:       op = "lo"; break;
            case PCREL_HI: op = "pcrel_hi"; break;
            case PCREL_LO: op = "pcrel_lo"; break;
            default:       unreachable();
        }
        std::string str = ptr->imm.to_string();
        if (str.starts_with('(') && str.ends_with(')'))
            return fmt::format("%{}{}", op, str);
        return fmt::format("%{}({})", op, str);
    } else if (auto ptr = dynamic_cast<TreeImmediate *>(imm)) {
        std::vector<std::string> vec;
        for (auto &[imm, op] : ptr->data) {
            std::string_view op_str = op == TreeImmediate::Operator::ADD ? " + "
                                    : op == TreeImmediate::Operator::SUB ? " - "
                                                                         : "";
            vec.push_back(fmt::format("{}{}", imm.to_string(), op_str));
        }
        std::size_t length{2};
        std::string string{'('};
        for (auto &str : vec)
            length += str.size();
        string.reserve(length);
        for (auto &str : vec)
            string += str;
        string += ')';
        return string;
    } else {
        unreachable();
    }
}

std::string Immediate::to_string() const {
    return imm_to_string(data.get());
}

} // namespace dark
