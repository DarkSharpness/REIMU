#include <utility.h>
#include <assembly/storage.h>
#include <assembly/exception.h>
#include <fmtlib>

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

auto sv_to_reg_nothrow(std::string_view view) noexcept -> std::optional<Register> {
using namespace ::dark::__hash;
    #define match_or_break(str, expr) case switch_hash_impl(str):\
        if (view == str) { return expr; } break

    using enum Register;

    switch (switch_hash_impl(view)) {
        match_or_break("zero", zero);
        match_or_break("ra",   ra);
        match_or_break("sp",   sp);
        match_or_break("gp",   gp);
        match_or_break("tp",   tp);
        match_or_break("t0",   t0);
        match_or_break("t1",   t1);
        match_or_break("t2",   t2);
        match_or_break("s0",   s0);
        match_or_break("s1",   s1);
        match_or_break("a0",   a0);
        match_or_break("a1",   a1);
        match_or_break("a2",   a2);
        match_or_break("a3",   a3);
        match_or_break("a4",   a4);
        match_or_break("a5",   a5);
        match_or_break("a6",   a6);
        match_or_break("a7",   a7);
        match_or_break("s2",   s2);
        match_or_break("s3",   s3);
        match_or_break("s4",   s4);
        match_or_break("s5",   s5);
        match_or_break("s6",   s6);
        match_or_break("s7",   s7);
        match_or_break("s8",   s8);
        match_or_break("s9",   s9);
        match_or_break("s10",  s10);
        match_or_break("s11",  s11);
        match_or_break("t3",   t3);
        match_or_break("t4",   t4);
        match_or_break("t5",   t5);
        match_or_break("t6",   t6);
        default: break;
    }

    #undef match_or_break

    if (view.starts_with("x")) {
        auto num = sv_to_integer <std::size_t> (view.substr(1));
        if (num.has_value() && *num < 32)
            return static_cast <Register> (*num);
    }

    return std::nullopt;
}

auto sv_to_reg(std::string_view view) -> Register {
    auto reg = sv_to_reg_nothrow(view);
    if (reg.has_value()) return *reg;
    throw FailToParse { std::format("Invalid register: \"{}\"", view) };
}

Alignment::Alignment(std::size_t alignment) : alignment(alignment) {
    throw_if(!std::has_single_bit(alignment), "Invalid alignment: \"{}\"", alignment);
}

IntegerData::IntegerData(std::string_view data, Type type)
    : data(data), type(type) {
    runtime_assert(Type::BYTE <= type && type <= Type::LONG);
}

ZeroBytes::ZeroBytes(std::size_t count) : count(count) {}

ASCIZ::ASCIZ(std::string str) : data(std::move(str)) {}

StrImmediate::StrImmediate(std::string_view data) : data(data) {
    for (char c : data) throw_if(!is_label_char(c), "Invalid label name: \"{}\"", data);
}

} // namespace dark
