#include <riscv/command.h>
#include <assembly/storage.h>
#include <linker/linker.h>
#include <linker/layout.h>
#include <linker/evaluate.h>
#include <linker/estimate.h>

namespace dark {

template <int _Nm>
static bool within_bits(target_size_t value) {
    static_assert(_Nm > 0 && _Nm < 32);
    constexpr auto min = static_cast <target_size_t> (-1) << (_Nm - 1);
    constexpr auto max = ~min;
    return value >= min && value <= max;
}

struct EncodingPass final : Evaluator, StorageVisitor {
    using Section = Linker::LinkResult::Section;
    Section &data;

    /**
     * An encoding pass which will encode actual command/data
     * into real binary data in form of byte array.
     */
    explicit EncodingPass(const _Table_t &global_table, 
        Section &data, const Linker::_Details_Vec_t &details)
            : Evaluator(global_table), data(data) {
        if (details.empty()) return;
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

    void align_to(target_size_t alignment) {
        runtime_assert(std::has_single_bit(alignment));
        auto bitmask = alignment - 1;
        auto current = this->get_current_position();
        auto new_pos = (current + bitmask) & ~bitmask;
        while (current++ < new_pos)
            data.storage.push_back(std::byte(0));
        this->set_position(new_pos);
    }

    auto imm_to_int(Immediate &imm) -> target_size_t {
        return this->evaluate(*imm.data);
    }

    void command_align() {
        panic_if(this->get_current_position() % alignof(command_size_t) != 0,
            "Command is not aligned.");
    }

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
        this->command_align();
        command::r_type cmd {};

        #define match_and_set(_Op_) \
            case (ArithmeticReg::Opcode::_Op_): \
                cmd.funct3 = command::r_type::Funct3::_Op_; \
                cmd.funct7 = command::r_type::Funct7::_Op_; \
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

        cmd.rd  = reg_to_int(storage.rd);
        cmd.rs1 = reg_to_int(storage.rs1);
        cmd.rs2 = reg_to_int(storage.rs2);

        this->push_command(cmd.to_integer());
    }

    void visitStorage(ArithmeticImm &storage) {
        this->command_align();
        using enum ArithmeticImm::Opcode;
        command::i_type cmd {};

        #define match_and_set(_Op_) \
            case (ArithmeticImm::Opcode::_Op_): \
                cmd.funct3 = command::i_type::Funct3::_Op_; \
                break

        switch (storage.opcode) {
            match_and_set(ADD);
            match_and_set(SLL);
            match_and_set(SLT);
            match_and_set(SLTU);
            match_and_set(XOR);
            match_and_set(SRL);
            match_and_set(SRA);
            match_and_set(OR);
            match_and_set(AND);

            default: unreachable();
        }

        #undef match_and_set

        cmd.rd  = reg_to_int(storage.rd);
        cmd.rs1 = reg_to_int(storage.rs1);
        auto imm = imm_to_int(storage.imm);

        if (storage.opcode != SRA) {
            cmd.set_imm(imm);
        } else { // SRA has a different encoding
            cmd.set_imm(imm | command::i_type::Funct7::SRA << 5);
        }

        this->push_command(cmd.to_integer());
    }

    void visitStorage(LoadStore &storage) {
        this->command_align();
        if (storage.is_load())  return this->visitLoad(storage);
        else                    return this->visitStore(storage);
        unreachable();
    }

    void visitLoad(LoadStore &storage) {
        using enum LoadStore::Opcode;
        command::l_type cmd {};

        #define match_and_set(_Op_) \
            case (LoadStore::Opcode::_Op_): \
                cmd.funct3 = command::l_type::Funct3::_Op_; \
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

        cmd.rd  = reg_to_int(storage.rd);
        cmd.rs1 = reg_to_int(storage.rs1);
        cmd.set_imm(imm_to_int(storage.imm));

        this->push_command(cmd.to_integer());
    }

    void visitStore(LoadStore &storage) {
        using enum LoadStore::Opcode;
        command::s_type cmd {};

        #define match_and_set(_Op_) \
            case (LoadStore::Opcode::_Op_): \
                cmd.funct3 = command::s_type::Funct3::_Op_; \
                break

        switch (storage.opcode) {
            match_and_set(SB);
            match_and_set(SH);
            match_and_set(SW);
            default: unreachable();
        }

        #undef match_and_set

        cmd.rs1 = reg_to_int(storage.rs1);
        cmd.rs2 = reg_to_int(storage.rd); // In fact rs2
        cmd.set_imm(imm_to_int(storage.imm));

        this->push_command(cmd.to_integer());
    }

    void visitStorage(Branch &storage) {
        this->command_align();
        using enum Branch::Opcode;
        command::b_type cmd {};

        #define match_and_set(_Op_) \
            case (Branch::Opcode::_Op_): \
                cmd.funct3 = command::b_type::Funct3::_Op_; \
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

        const auto target   = imm_to_int(storage.imm);
        const auto distance = target - this->get_current_position();

        cmd.rs1 = reg_to_int(storage.rs1);
        cmd.rs2 = reg_to_int(storage.rs2);
        cmd.set_imm(distance);

        this->push_command(cmd.to_integer());
    }

    void visitStorage(JumpRelative &storage) {
        this->command_align();
        command::jal cmd {};

        const auto target   = imm_to_int(storage.imm);
        const auto distance = target - this->get_current_position();

        cmd.rd = reg_to_int(storage.rd);
        cmd.set_imm(distance);

        this->push_command(cmd.to_integer());
    }

    void visitStorage(JumpRegister &storage) {
        this->command_align();
        command::jalr cmd {};

        cmd.rd  = reg_to_int(storage.rd);
        cmd.rs1 = reg_to_int(storage.rs1);
        cmd.set_imm(imm_to_int(storage.imm));

        this->push_command(cmd.to_integer());
    }

    void visitStorage(CallFunction &storage) {
        this->command_align();
        // Into 2 commands: auipc and jalr
        const auto target = imm_to_int(storage.imm);
        const auto distance = target - this->get_current_position();

        const auto [lo, hi] = split_lo_hi(distance);

        command::auipc  cmd_0 {};
        command::jalr   cmd_1 {};

        cmd_0.set_imm(hi);
        cmd_1.set_imm(lo);

        using enum Register;

        // This is specified by RISC-V spec
        const auto [tmp_reg, ret_reg] =
            storage.is_tail_call() ?
                std::make_pair(t1, zero) : std::make_pair(ra, ra);

        cmd_0.rd    = reg_to_int(tmp_reg);
        cmd_1.rs1   = reg_to_int(tmp_reg);
        cmd_1.rd    = reg_to_int(ret_reg);

        this->push_command(cmd_0.to_integer());
        this->push_command(cmd_1.to_integer());
    }

    void visitStorage(LoadImmediate &storage) {
        this->command_align();
        const auto imm  = imm_to_int(storage.imm);
        const auto rd   = reg_to_int(storage.rd);

        const auto [lo, hi] = split_lo_hi(imm);

        command::lui    cmd_0 {};
        command::i_type cmd_1 {};

        cmd_0.rd = rd;
        cmd_0.set_imm(hi);
        cmd_1.funct3 = command::i_type::Funct3::ADD;
        cmd_1.rd = rd;
        cmd_1.rs1 = rd;
        cmd_1.set_imm(lo);

        this->push_command(cmd_0.to_integer());
        this->push_command(cmd_1.to_integer());
    }

    void visitStorage(LoadUpperImmediate &storage) {
        this->command_align();
        command::lui cmd {};

        cmd.rd = reg_to_int(storage.rd);
        cmd.set_imm(imm_to_int(storage.imm));

        this->push_command(cmd.to_integer());
    }

    void visitStorage(AddUpperImmediatePC &storage) {
        this->command_align();
        command::auipc cmd {};

        cmd.rd = reg_to_int(storage.rd);
        cmd.set_imm(imm_to_int(storage.imm));

        this->push_command(cmd.to_integer());
    }

    void visitStorage(Alignment &storage) {
        this->align_to(__details::align_size(storage));
        runtime_assert(__details::real_size(storage) == 0);
    }

    void visitStorage(IntegerData &storage) {
        this->align_to(__details::align_size(storage));
        using enum IntegerData::Type;
        switch (auto data = imm_to_int(storage.data); storage.type) {
            case BYTE : this->push_byte(data); break;
            case SHORT: this->push_half(data); break;
            case LONG : this->push_word(data); break;
        }
    }

    void visitStorage(ZeroBytes &storage) {
        this->align_to(__details::align_size(storage));
        auto size = __details::real_size(storage);
        while (size --> 0) this->push_byte(0);
    }

    void visitStorage(ASCIZ &storage) {
        this->align_to(__details::align_size(storage));
        auto size = __details::real_size(storage);
        // Including null-terminator
        for (std::size_t i = 0; i < size; ++i)
            this->push_byte(storage.data[i]);
    }
};

static void connect(EncodingPass::Section &prev, EncodingPass::Section &next) {
    if (next.storage.empty())
        next.start = prev.start + prev.storage.size();
}

/**
 * Link these targeted files.
 * It will translate all symbols into integer constants.
 */
void Linker::link() {
    auto &result = this->result.emplace <MemoryLayout> ();

    for (auto &[name, location] : this->global_symbol_table)
        result.position_table.emplace(name, location.get_location());

    EncodingPass(this->global_symbol_table, result.text, this->get_section(Section::TEXT));
    EncodingPass(this->global_symbol_table, result.data, this->get_section(Section::DATA));
    EncodingPass(this->global_symbol_table, result.rodata, this->get_section(Section::RODATA));
    EncodingPass(this->global_symbol_table, result.unknown, this->get_section(Section::UNKNOWN));
    EncodingPass(this->global_symbol_table, result.bss, this->get_section(Section::BSS));

    connect(result.text, result.data);
    connect(result.data, result.rodata);
    connect(result.rodata, result.unknown);
    connect(result.unknown, result.bss);
}

/** Get the result of linking. */
auto Linker::get_linked_layout() && -> LinkResult {
    return std::move(this->result.get <LinkResult &> ());
}

} // namespace dark
