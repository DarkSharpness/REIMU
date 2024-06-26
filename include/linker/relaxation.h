// Should only be included once by linker.cpp
#include "trivial.h"
#include "evaluate.h"
#include <utility.h>
#include <storage.h>
#include <ustring.h>

namespace dark {

struct RelaxtionPass final : private Evaluator, StorageVisitor {
  private:
    _Storage_t retval;

  public:
    /**
     * A relaxation pass which can patch up the code to make it more efficient.
     * 
     * It will turn some of the calls into jumps, and try to shrink the code
     * size without changing the semantics of the code.
     */
    explicit RelaxtionPass(const _Table_t &global_table, const Linker::_Details_Vec_t &vec)
        : Evaluator(global_table) {
        for (auto &details : vec) {
            this->set_local(details.table);
            std::size_t index   = 0;
            std::size_t current = details.begin_position;
            for (auto &storage : details.storage) {
                this->set_position(current + details.offsets[index++]);
                this->visit(*storage);
                if (retval) storage = std::move(retval);
            }
        }
    }

  private:

    void visitStorage(ArithmeticReg &storage)       override {}
    void visitStorage(ArithmeticImm &storage)       override { TrivialPass{storage.imm}; }
    void visitStorage(LoadStore &storage)           override { TrivialPass{storage.imm}; }
    void visitStorage(Branch &storage)              override { TrivialPass{storage.imm}; }
    void visitStorage(JumpRelative &storage)        override { TrivialPass{storage.imm}; }
    void visitStorage(JumpRegister &storage)        override { TrivialPass{storage.imm}; }
    void visitStorage(LoadUpperImmediate &storage)  override { TrivialPass{storage.imm}; }
    void visitStorage(AddUpperImmediatePC &storage) override { TrivialPass{storage.imm}; }
    void visitStorage(Alignment &storage)           override {}
    void visitStorage(IntegerData &storage)         override { TrivialPass{storage.data}; }
    void visitStorage(ZeroBytes &storage)           override {}
    void visitStorage(ASCIZ &storage)               override {}

    void visitStorage(CallFunction &storage) override
    { TrivialPass{storage.imm}; return visit_call(storage); }

    void visitStorage(LoadImmediate &storage) override
    { TrivialPass{storage.imm}; return visit_li(storage); }

    void visit_call(CallFunction &call) {
        auto &imm = call.imm.data;
        if (!dynamic_cast <StrImmediate *> (imm.get())) return;

        auto &str = static_cast <StrImmediate &> (*imm);
        auto current = Evaluator::get_current_location();
        auto [absolute, relative] = Evaluator::get_symbol_position(str.data.to_sv());
        auto destination = absolute + relative;

        // Maybe we should not optimize libc calls.
        // if (destination <= libc::kLibcEnd) return;

        auto distance = static_cast <target_ssize_t> (destination - current);

        // We relax conservatively, in hope that after the relaxation,
        // an old jump will not go invalid due to the relocation.
        // If still crash, we may roll back and give up the relaxation.

        constexpr auto kJumpMax = ((target_ssize_t {1} << 19) - 1);
        constexpr auto kJumpMin = ((target_ssize_t {1} << 19) * -1);

        if (kJumpMin / 2 <= distance && distance <= kJumpMax / 2) {
            using enum Register;
            if (call.tail) {
                retval = std::make_unique <JumpRelative> (zero, std::move(call.imm));
            } else {
                retval = std::make_unique <JumpRelative> (ra, std::move(call.imm));
            }
        }
    }

    void visit_li(LoadImmediate &load) {
        auto &imm = load.imm.data;
        if (!dynamic_cast <IntImmediate *> (imm.get())) return;
        auto &integer = static_cast <IntImmediate &> (*imm);
        auto value = static_cast <target_ssize_t> (integer.data);

        constexpr auto kAddiMax = (target_ssize_t {1} << 11) - 1;
        constexpr auto kAddiMin = (target_ssize_t {1} << 11) * -1;
        constexpr auto kLuiUnit = target_size_t {1} << 12;

        if (kAddiMin <= value && value <= kAddiMax) {
            using enum Register;
            using enum ArithmeticImm::Opcode;
            retval = std::make_unique <ArithmeticImm> (ADD, load.rd, zero, std::move(load.imm));
        } else if (integer.data % kLuiUnit == 0) {
            integer.data /= kLuiUnit;
            retval = std::make_unique <LoadUpperImmediate> (load.rd, std::move(load.imm));
        }
    }
};


} // namespace dark