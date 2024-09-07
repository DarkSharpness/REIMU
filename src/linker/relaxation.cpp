#include "assembly/storage/command.h"
#include "assembly/storage/static.h"
#include "assembly/storage/visitor.h"
#include "linker/evaluate.h"
#include "linker/linker.h"
#include "utility/cast.h"
#include "utility/misc.h"

namespace dark {

struct TrivialPass {
public:
    /**
     * Trivial pass is a simple rewriter that can rewrite any constant
     *
     * It will try to rewrite any location independent constant into a
     * direct integer constant.
     *
     * It will also perform some simple optimizations on the tree structure,
     * such as folding the one-element tree.
     */
    template <typename... Args>
        requires(std::is_same_v<Args, Immediate> && ...)
    explicit TrivialPass(Args &...args) {
        [[maybe_unused]]
        bool result = (this->evaluate(args) & ...);
    }

private:
    /* Rewrite an immediate with given constant. */
    static void rewrite(Immediate &data, target_size_t value) {
        data.data = std::make_unique<IntImmediate>(value);
    }

    /* Get the integer value of an immediate. */
    static target_size_t get_integer(Immediate &data) {
        return dynamic_cast<IntImmediate &>(*data.data).data;
    }

    /* Move out the integer value and transform. */
    template <typename _Fn>
    static Immediate move_integer(Immediate &data, _Fn &&fn) {
        auto &imm = dynamic_cast<IntImmediate &>(*data.data);
        imm.data  = fn(imm.data);
        return std::move(data);
    }

    /* Evaluate a tree immediate. */
    static bool evaluate_tree(Immediate &data) {
        auto &tree = dynamic_cast<TreeImmediate &>(*data.data);

        if (tree.data.size() == 1) {
            /**
             * This is a hard buggy case, which does not exist in rust.
             * When moving from a member to the parent, the parent may
             * be destructed before the member is moved.
             * So, to ensure the correctness, we need to move the member
             * out of the parent first, then move the member to the parent.
             */
            auto imm = std::move(tree.data[0].imm);
            return evaluate(data = std::move(imm));
        }

        using enum TreeImmediate::Operator;
        auto last_op = ADD;

        bool success = true;
        auto result  = target_size_t{0};

        for (auto &[sub, op] : tree.data) {
            if (!evaluate(sub) || !success) {
                success = false;
            } else { // All evaluations are successful.
                auto value = get_integer(sub);
                switch (last_op) {
                    case ADD: result += value; break;
                    case SUB: result -= value; break;
                    default:  unreachable();
                }
            }
            last_op = op;
        }

        runtime_assert(last_op == END);
        return success ? (rewrite(data, result), true) : false;
    }

    /* Whether the child can be rewritten into a integer. */
    static bool evaluate(Immediate &data) {
        auto &imm = *data.data;

        if (dynamic_cast<IntImmediate *>(&imm))
            return true;

        if (dynamic_cast<StrImmediate *>(&imm))
            return false;

        if (auto *relative = dynamic_cast<RelImmediate *>(&imm)) {
            switch (relative->operand) {
                using enum RelImmediate::Operand;
                case HI:
                    if (!evaluate(relative->imm))
                        return false;
                    data = move_integer(relative->imm, [](auto x) { return split_lo_hi(x).hi; });
                    return true;
                case LO:
                    if (!evaluate(relative->imm))
                        return false;
                    data = move_integer(relative->imm, [](auto x) { return split_lo_hi(x).lo; });
                    return true;
                default: return false;
            }
        }

        return evaluate_tree(data);
    }
};

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
    explicit RelaxtionPass(const _Table_t &global_table, const Linker::_Details_Vec_t &vec) :
        Evaluator(global_table) {
        for (auto &details : vec) {
            this->set_local(details.get_local_table());
            for (auto &&[storage, position] : details) {
                this->set_position(position);
                this->visit(*storage);
                if (retval)
                    storage = std::move(retval);
            }
        }
    }

private:
    // clang-format off

    void visitStorage(ArithmeticReg &storage)       override { allow_unused(storage); }
    void visitStorage(ArithmeticImm &storage)       override { TrivialPass{storage.imm}; }
    void visitStorage(LoadStore &storage)           override { TrivialPass{storage.imm}; }
    void visitStorage(Branch &storage)              override { TrivialPass{storage.imm}; }
    void visitStorage(JumpRelative &storage)        override { TrivialPass{storage.imm}; }
    void visitStorage(JumpRegister &storage)        override { TrivialPass{storage.imm}; }
    void visitStorage(LoadUpperImmediate &storage)  override { TrivialPass{storage.imm}; }
    void visitStorage(AddUpperImmediatePC &storage) override { TrivialPass{storage.imm}; }
    void visitStorage(Alignment &storage)           override { allow_unused(storage); }
    void visitStorage(IntegerData &storage)         override { TrivialPass{storage.data}; }
    void visitStorage(ZeroBytes &storage)           override { allow_unused(storage); }
    void visitStorage(ASCIZ &storage)               override { allow_unused(storage); }

    // clang-format on

    void visitStorage(CallFunction &storage) override {
        TrivialPass{storage.imm};
        return visit_call(storage);
    }

    void visitStorage(LoadImmediate &storage) override {
        TrivialPass{storage.imm};
        return visit_li(storage);
    }

    void visit_call(CallFunction &call) {
        auto &imm = call.imm.data;
        // if (!dynamic_cast <StrImmediate *> (imm.get())) return;
        // auto &str = static_cast <StrImmediate &> (*imm);

        auto current     = Evaluator::get_current_position();
        auto destination = Evaluator::evaluate(*imm);

        // auto destination = Evaluator::get_symbol_position(str.data.to_sv());

        // Maybe we should not optimize libc calls.
        // if (destination <= libc::kLibcEnd) return;

        auto distance = static_cast<target_ssize_t>(destination - current);

        // We relax conservatively, in hope that after the relaxation,
        // an old jump will not go invalid due to the relocation.
        // If still crash, we may roll back and give up the relaxation.

        constexpr auto kJumpMax = ((target_ssize_t{1} << 19) - 1);
        constexpr auto kJumpMin = ((target_ssize_t{1} << 19) * -1);

        if (kJumpMin / 2 <= distance && distance <= kJumpMax / 2) {
            using enum Register;
            if (call.is_tail_call()) {
                retval = std::make_unique<JumpRelative>(zero, std::move(call.imm));
            } else {
                retval = std::make_unique<JumpRelative>(ra, std::move(call.imm));
            }
        }
    }

    void visit_li(LoadImmediate &load) {
        auto &imm = load.imm.data;
        if (!dynamic_cast<IntImmediate *>(imm.get()))
            return;
        auto &integer = static_cast<IntImmediate &>(*imm);
        auto value    = static_cast<target_ssize_t>(integer.data);

        constexpr auto kAddiMax = (target_ssize_t{1} << 11) - 1;
        constexpr auto kAddiMin = (target_ssize_t{1} << 11) * -1;
        constexpr auto kLuiUnit = target_size_t{1} << 12;

        if (kAddiMin <= value && value <= kAddiMax) {
            using enum Register;
            using enum ArithmeticImm::Opcode;
            retval = std::make_unique<ArithmeticImm>(ADD, load.rd, zero, std::move(load.imm));
        } else if (integer.data % kLuiUnit == 0) {
            integer.data /= kLuiUnit;
            retval = std::make_unique<LoadUpperImmediate>(load.rd, std::move(load.imm));
        }
    }
};

void Linker::make_relaxation() {
    for (auto &vec : this->details_vec)
        RelaxtionPass(this->global_symbol_table, vec);
}

} // namespace dark
