#include "assembly/forward.h"
#include "assembly/storage/command.h"
#include "assembly/storage/static.h"
#include "assembly/storage/visitor.h"
#include "declarations.h"
#include "fmtlib.h"
#include "libc/libc.h"
#include "linker/estimate.h"
#include "linker/evaluate.h"
#include "linker/layout.h"
#include "linker/linker.h"
#include "riscv/command.h"
#include "utility/error.h"
#include <cctype>
#include <string>
#include <string_view>

namespace dark {

struct FailToLink {
    std::string what;
};

template <int _Nm, bool _Signed>
static auto check_bits(target_size_t imm, std::string_view name) -> command_size_t {
    static_assert(0 < _Nm && _Nm < 32, "Invalid bit width");
    constexpr auto min = _Signed ? target_size_t(-1) << (_Nm - 1) : 0;
    constexpr auto max = _Signed ? ~min : (target_size_t(1) << _Nm) - 1;

    if ((imm - min) <= (max - min)) [[likely]]
        return imm;

    // From upper to lower
    std::string name_str{name};
    for (auto &ch : name_str)
        ch = std::tolower(ch);

    throw FailToLink{fmt::format(
        "\"{}\" immediate out of range, should be within [{}, {}]", name_str,
        static_cast<target_ssize_t>(min), max
    )};
}

struct Encoder final : Evaluator, StorageVisitor {
public:
    using Section = Linker::LinkResult::Section;

    /**
     * An encoding pass which will encode actual command/data
     * into real binary data in form of byte array.
     */
    explicit Encoder(
        const _Table_t &global_table, Section &data, const Linker::_Details_Vec_t &details
    ) : Evaluator(global_table), data(data) {
        if (details.empty())
            return;
        data.start = details.front().get_start();
        for (auto &detail : details) {
            this->set_local(detail.get_local_table());
            this->set_position(detail.get_start());
            for (auto &&[storage, position] : detail) {
                runtime_assert(position == this->get_current_position());
                this->visit(*storage);
            }
        }
    }

private:
    Section &data;

    void visit(Storage &storage) {
        try {
            if (dynamic_cast<Command *>(&storage))
                this->check_command();
            StorageVisitor::visit(storage);
            /// TODO: catch the exception, and format the error message
            /// Plan to add detailed information about where crashed
        } catch (FailToLink &e) {
            panic("Fail to link source assembly.\n  {}", e.what);
        } catch (...) { unreachable("Unknown exception caught in Encoder::visit"); }
    }

    void check_command() {
        if (this->get_current_position() % alignof(command_size_t) == 0) [[likely]]
            return;

        throw FailToLink{
            fmt::format("Command is not aligned (should align to {})", alignof(command_size_t))
        };
    }

    void check_alignment(target_size_t alignment) {
        runtime_assert(std::has_single_bit(alignment));
        if (this->get_current_position() % alignment == 0)
            return;

        throw FailToLink{fmt::format("Static data is not aligned (should align to {})", alignment)};
    }

    auto imm_to_int(Immediate &imm) -> target_size_t { return this->evaluate(*imm.data); }

    void push_byte(std::uint8_t byte) {
        data.storage.push_back(std::byte(byte));
        this->inc_position(1);
    }

    void push_half(std::uint16_t half) {
        data.storage.push_back(std::byte((half >> 0) & 0xFF));
        data.storage.push_back(std::byte((half >> 8) & 0xFF));
        this->inc_position(2);
    }

    void push_word(std::uint32_t word) {
        data.storage.push_back(std::byte((word >> 0) & 0xFF));
        data.storage.push_back(std::byte((word >> 8) & 0xFF));
        data.storage.push_back(std::byte((word >> 16) & 0xFF));
        data.storage.push_back(std::byte((word >> 24) & 0xFF));
        this->inc_position(4);
    }

    void push_command(command_size_t raw) {
        static_assert(sizeof(raw) == 4);
        return this->push_word(raw);
    }

    void visitStorage(ArithmeticReg &storage) {
        command::r_type cmd{};

        cmd.rd  = reg_to_int(storage.rd);
        cmd.rs1 = reg_to_int(storage.rs1);
        cmd.rs2 = reg_to_int(storage.rs2);

#define match_and_set(_Op_)                                                                        \
    case (ArithmeticReg::Opcode::_Op_):                                                            \
        cmd.funct3 = command::r_type::Funct3::_Op_;                                                \
        cmd.funct7 = command::r_type::Funct7::_Op_;                                                \
        break

        switch (storage.opcode) {
            match_and_set(ADD);
            match_and_set(SUB);
            match_and_set(SLL);
            match_and_set(SLT);
            match_and_set(SLTU);
            match_and_set(XOR);
            match_and_set(SRL);
            match_and_set(SRA);
            match_and_set(OR);
            match_and_set(AND);

            match_and_set(MUL);
            match_and_set(MULH);
            match_and_set(MULHSU);
            match_and_set(MULHU);
            match_and_set(DIV);
            match_and_set(DIVU);
            match_and_set(REM);
            match_and_set(REMU);

            default: unreachable();
        }
#undef match_and_set

        this->push_command(cmd.to_integer());
    }

    void visitStorage(ArithmeticImm &storage) {
        command::i_type cmd{};

        cmd.rd   = reg_to_int(storage.rd);
        cmd.rs1  = reg_to_int(storage.rs1);
        auto imm = imm_to_int(storage.imm);

#define match_and_set(_Op_, len, flag, str)                                                        \
    case (ArithmeticImm::Opcode::_Op_):                                                            \
        cmd.funct3 = command::i_type::Funct3::_Op_;                                                \
        cmd.set_imm(check_bits<len, flag>(imm, str) | command::i_type::Funct7::_Op_ << len);       \
        break

        switch (storage.opcode) {
            match_and_set(ADD, 12, true, "addi");
            match_and_set(SLL, 5, false, "slli");
            match_and_set(SLT, 12, true, "slti");
            match_and_set(SLTU, 12, true, "sltiu");
            match_and_set(XOR, 12, true, "xori");
            match_and_set(SRL, 5, false, "srli");
            match_and_set(SRA, 5, false, "srai");
            match_and_set(OR, 12, true, "ori");
            match_and_set(AND, 12, true, "andi");

            default: unreachable();
        }
#undef match_and_set

        this->push_command(cmd.to_integer());
    }

    void visitStorage(LoadStore &storage) {
        if (storage.is_load())
            return this->visitLoad(storage);
        else
            return this->visitStore(storage);
    }

    void visitLoad(LoadStore &storage) {
        command::l_type cmd{};
        cmd.rd   = reg_to_int(storage.rd);
        cmd.rs1  = reg_to_int(storage.rs1);
        auto imm = imm_to_int(storage.imm);

#define match_and_set(_Op_)                                                                        \
    case (LoadStore::Opcode::_Op_):                                                                \
        cmd.funct3 = command::l_type::Funct3::_Op_;                                                \
        cmd.set_imm(check_bits<12, true>(imm, #_Op_));                                             \
        break

        switch (storage.opcode) {
            match_and_set(LB);
            match_and_set(LH);
            match_and_set(LW);
            match_and_set(LBU);
            match_and_set(LHU);
            default: unreachable();
        }
#undef match_and_set

        this->push_command(cmd.to_integer());
    }

    void visitStore(LoadStore &storage) {
        command::s_type cmd{};
        cmd.rs1  = reg_to_int(storage.rs1);
        cmd.rs2  = reg_to_int(storage.rd); // In fact rs2
        auto imm = imm_to_int(storage.imm);

#define match_and_set(_Op_)                                                                        \
    case (LoadStore::Opcode::_Op_):                                                                \
        cmd.funct3 = command::s_type::Funct3::_Op_;                                                \
        cmd.set_imm(check_bits<12, true>(imm, #_Op_));                                             \
        break

        switch (storage.opcode) {
            match_and_set(SB);
            match_and_set(SH);
            match_and_set(SW);
            default: unreachable();
        }
#undef match_and_set

        this->push_command(cmd.to_integer());
    }

    void visitStorage(Branch &storage) {
        command::b_type cmd{};

        const auto target   = imm_to_int(storage.imm);
        const auto distance = target - this->get_current_position();

        cmd.rs1 = reg_to_int(storage.rs1);
        cmd.rs2 = reg_to_int(storage.rs2);

#define match_and_set(_Op_)                                                                        \
    case (Branch::Opcode::_Op_):                                                                   \
        cmd.funct3 = command::b_type::Funct3::_Op_;                                                \
        cmd.set_imm(check_bits<13, true>(distance, #_Op_));                                        \
        break

        switch (storage.opcode) {
            match_and_set(BEQ);
            match_and_set(BNE);
            match_and_set(BLT);
            match_and_set(BGE);
            match_and_set(BLTU);
            match_and_set(BGEU);
            default: unreachable();
        }
#undef match_and_set

        this->push_command(cmd.to_integer());
    }

    void visitStorage(JumpRelative &storage) {
        command::jal cmd{};

        const auto target   = imm_to_int(storage.imm);
        const auto distance = target - this->get_current_position();

        cmd.rd = reg_to_int(storage.rd);
        cmd.set_imm(check_bits<21, true>(distance, "jal"));

        this->push_command(cmd.to_integer());
    }

    void visitStorage(JumpRegister &storage) {
        command::jalr cmd{};

        cmd.rd  = reg_to_int(storage.rd);
        cmd.rs1 = reg_to_int(storage.rs1);

        cmd.set_imm(check_bits<12, true>(imm_to_int(storage.imm), "jalr"));

        this->push_command(cmd.to_integer());
    }

    void visitStorage(CallFunction &storage) {
        // Into 2 commands: auipc and jalr
        const auto target   = imm_to_int(storage.imm);
        const auto distance = target - this->get_current_position();

        const auto [lo, hi] = split_lo_hi(distance);

        command::auipc cmd_0{};
        command::jalr cmd_1{};

        // No need to check the range of distance, always safe
        cmd_0.set_imm(hi);
        cmd_1.set_imm(lo);

        using enum Register;

        // This is specified by RISC-V spec
        const auto [tmp_reg, ret_reg] =
            storage.is_tail_call() ? std::make_pair(t1, zero) : std::make_pair(ra, ra);

        cmd_0.rd  = reg_to_int(tmp_reg);
        cmd_1.rs1 = reg_to_int(tmp_reg);
        cmd_1.rd  = reg_to_int(ret_reg);

        this->push_command(cmd_0.to_integer());
        this->push_command(cmd_1.to_integer());
    }

    void visitStorage(LoadImmediate &storage) {
        const auto imm = imm_to_int(storage.imm);
        const auto rd  = reg_to_int(storage.rd);

        const auto [lo, hi] = split_lo_hi(imm);

        command::lui cmd_0{};
        command::i_type cmd_1{};

        // No need to check the range of imm, always safe

        cmd_0.rd = rd;
        cmd_0.set_imm(hi);
        cmd_1.funct3 = command::i_type::Funct3::ADD;
        cmd_1.rd     = rd;
        cmd_1.rs1    = rd;
        cmd_1.set_imm(lo);

        this->push_command(cmd_0.to_integer());
        this->push_command(cmd_1.to_integer());
    }

    void visitStorage(LoadUpperImmediate &storage) {
        command::lui cmd{};

        cmd.rd = reg_to_int(storage.rd);
        cmd.set_imm(check_bits<20, false>(imm_to_int(storage.imm), "lui"));

        this->push_command(cmd.to_integer());
    }

    void visitStorage(AddUpperImmediatePC &storage) {
        command::auipc cmd{};

        cmd.rd = reg_to_int(storage.rd);
        cmd.set_imm(check_bits<20, false>(imm_to_int(storage.imm), "auipc"));

        this->push_command(cmd.to_integer());
    }

    void visitStorage(Alignment &storage) {
        auto alignment = __details::align_size(storage);
        runtime_assert(std::has_single_bit(alignment));
        auto bitmask = alignment - 1;
        auto current = this->get_current_position();
        auto new_pos = (current + bitmask) & ~bitmask;
        while (current++ < new_pos)
            data.storage.push_back(std::byte(0));
        this->set_position(new_pos);

        runtime_assert(__details::real_size(storage) == 0);
    }

    void visitStorage(IntegerData &storage) {
        this->check_alignment(__details::align_size(storage));
        using enum IntegerData::Type;
        switch (auto data = imm_to_int(storage.data); storage.type) {
            case BYTE:  this->push_byte(data); break;
            case SHORT: this->push_half(data); break;
            case LONG:  this->push_word(data); break;
            default:    unreachable();
        }
    }

    void visitStorage(ZeroBytes &storage) {
        this->check_alignment(__details::align_size(storage));
        auto size = __details::real_size(storage);
        while (size-- > 0)
            this->push_byte(0);
    }

    void visitStorage(ASCIZ &storage) {
        this->check_alignment(__details::align_size(storage));
        auto size = __details::real_size(storage);
        // Including null-terminator
        for (std::size_t i = 0; i < size; ++i)
            this->push_byte(storage.data[i]);
    }
};

template <typename... _Args>
static void encoding_pass(_Args &&...args) {
    [[maybe_unused]]
    Encoder _{std::forward<_Args>(args)...};
}

static void connect(Encoder::Section &prev, Encoder::Section &next) {
    if (next.storage.empty())
        next.start = prev.start + prev.storage.size();
}

/**
 * Link these targeted files.
 * It will translate all symbols into integer constants.
 */
void Linker::link() {
    auto &result = this->result.emplace<MemoryLayout>();

    for (auto &[name, location] : this->global_symbol_table)
        result.position_table.emplace(name, location.get_location());

    auto &table = this->global_symbol_table;

    result.text.start = libc::kLibcEnd;

    encoding_pass(table, result.text, this->get_section(Section::TEXT));
    encoding_pass(table, result.data, this->get_section(Section::DATA));
    encoding_pass(table, result.rodata, this->get_section(Section::RODATA));
    encoding_pass(table, result.unknown, this->get_section(Section::UNKNOWN));
    encoding_pass(table, result.bss, this->get_section(Section::BSS));

    runtime_assert(result.text.start == libc::kLibcEnd);

    connect(result.text, result.data);
    connect(result.data, result.rodata);
    connect(result.rodata, result.unknown);
    connect(result.unknown, result.bss);
}

/** Get the result of linking. */
auto Linker::get_linked_layout() && -> LinkResult {
    return std::move(this->result.get<LinkResult &>());
}

} // namespace dark
