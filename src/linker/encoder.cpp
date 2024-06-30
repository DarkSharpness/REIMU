#include <riscv/command.h>
#include <assembly/storage.h>
#include <linker/linker.h>
#include <linker/evaluate.h>
#include <linker/estimate.h>

namespace dark {

struct EvaluationPass final : Evaluator, StorageVisitor {
  public:
    /**
     * A pass which will replace all the immediate values with their actual values.
     * After this pass, all immediate values will be replaced with integer constants.
     */
    explicit EvaluationPass(const _Table_t &global_table, const Linker::_Details_Vec_t &vec)
        : Evaluator(global_table) {
        for (auto &details : vec) {
            this->set_local(details.get_local_table());
            for (auto &&[storage, position] : details) {
                this->set_position(position);
                this->visit(*storage);
            }
        }
    }

    void evaluate(Immediate &imm) {
        auto value = Evaluator::evaluate(*imm.data);
        imm.data = std::make_unique <IntImmediate> (value);
    }

  private:

    void visitStorage(ArithmeticReg &storage)       override {}
    void visitStorage(ArithmeticImm &storage)       override { this->evaluate(storage.imm); }
    void visitStorage(LoadStore &storage)           override { this->evaluate(storage.imm); }
    void visitStorage(Branch &storage)              override { this->evaluate(storage.imm); }
    void visitStorage(JumpRelative &storage)        override { this->evaluate(storage.imm); }
    void visitStorage(JumpRegister &storage)        override { this->evaluate(storage.imm); }
    void visitStorage(CallFunction &storage)        override { this->evaluate(storage.imm); }
    void visitStorage(LoadImmediate &storage)       override { this->evaluate(storage.imm); }
    void visitStorage(LoadUpperImmediate &storage)  override { this->evaluate(storage.imm); }
    void visitStorage(AddUpperImmediatePC &storage) override { this->evaluate(storage.imm); }
    void visitStorage(Alignment &storage)           override {}
    void visitStorage(IntegerData &storage)         override { this->evaluate(storage.data); }
    void visitStorage(ZeroBytes &storage)           override {}
    void visitStorage(ASCIZ &storage)               override {}
};


struct EncodingPass final : StorageVisitor {
    using Section_t = Linker::LinkResult::Section;
    Section_t &data;
    std::size_t position;

    /**
     * An encoding pass which will encode actual command/data
     * into real binary data in form of byte array.
     */
    explicit EncodingPass(Section_t &data, const Linker::_Details_Vec_t &details) : data(data) {
        if (details.empty()) return;
        data.start = details.front().get_start();
        for (auto &detail : details) {
            this->position = detail.get_start();
            for (auto &&[storage, position] : detail) {
                runtime_assert(position == this->position);
                this->visit(*storage);
            }
        }
    }

  private:

    void align_to(std::size_t alignment) {
        runtime_assert(std::has_single_bit(alignment));
        auto bitmask = alignment - 1;
        auto current = this->position;
        auto new_pos = (current + bitmask) & ~bitmask;
        for (std::size_t i = current; i < new_pos; ++i)
            data.storage.push_back(std::byte(0));
        this->position = new_pos;
    }

    static auto imm_to_int(Immediate &imm) -> target_size_t {
        return dynamic_cast <IntImmediate &> (*imm.data).data;
    }

    void command_align() {
        panic_if(position % 4 != 0, "Command is not aligned");
    }

    void push_byte(std::uint8_t byte) {
        data.storage.push_back(std::byte(byte));
        position += 1;
    }

    void push_half(std::uint16_t half) {
        data.storage.push_back(std::byte((half >> 0) & 0xFF));
        data.storage.push_back(std::byte((half >> 8) & 0xFF));
        position += 2;
    }

    void push_word(std::uint32_t word) {
        data.storage.push_back(std::byte((word >> 0) & 0xFF));
        data.storage.push_back(std::byte((word >> 8) & 0xFF));
        data.storage.push_back(std::byte((word >> 16) & 0xFF));
        data.storage.push_back(std::byte((word >> 24) & 0xFF));
        position += 4;
    }

    void push_command(std::uint32_t raw) { return this->push_word(raw); }

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
            default: runtime_assert(false);
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
            default: runtime_assert(false);
        }

        #undef match_and_set

        cmd.rd  = reg_to_int(storage.rd);
        cmd.rs1 = reg_to_int(storage.rs1);
        cmd.set_imm(imm_to_int(storage.imm));

        this->push_command(cmd.to_integer());
    }

    void visitStorage(LoadStore &storage) {
        this->command_align();
        if (storage.is_load())  return this->visitLoad(storage);
        else                    return this->visitStore(storage);
        runtime_assert(false);
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
            default: runtime_assert(false);
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
            default: runtime_assert(false);
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
            default: runtime_assert(false);
        }

        #undef match_and_set

        cmd.rs1 = reg_to_int(storage.rs1);
        cmd.rs2 = reg_to_int(storage.rs2);
        cmd.set_imm(imm_to_int(storage.imm));

        this->push_command(cmd.to_integer());
    }

    void visitStorage(JumpRelative &storage) {
        this->command_align();
        command::jal cmd {};

        cmd.rd  = reg_to_int(storage.rd);
        cmd.set_imm(imm_to_int(storage.imm));

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
        const auto pc = this->position;
        const auto target = imm_to_int(storage.imm);
        const auto distance = target - pc;

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
        cmd_1.funct3 = 0b011;
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
        while (size --> 0) this->push_byte(storage.data[size]);
    }
};

/**
 * Link these targeted files.
 * It will translate all symbols into integer constants.
 */
void Linker::link() {
    for (auto &vec : this->details_vec)
        EvaluationPass(this->global_symbol_table, vec);

    auto &result = this->result.emplace();

    for (auto &[name, location] : this->global_symbol_table)
        result.position_table.emplace(name, location.get_location());

    EncodingPass(result.text, this->get_section(Section::TEXT));
    EncodingPass(result.data, this->get_section(Section::DATA));
    EncodingPass(result.rodata, this->get_section(Section::RODATA));
    EncodingPass(result.unknown, this->get_section(Section::UNKNOWN));
    EncodingPass(result.bss, this->get_section(Section::BSS));
}

} // namespace dark
