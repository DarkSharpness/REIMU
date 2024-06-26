// Should only be included once by relaxtion.h
#include "linker.h"
#include <utility.h>
#include <storage.h>
#include <ustring.h>

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
    template <typename ...Args>
    requires (std::is_same_v <Args, Immediate> && ...)
    explicit TrivialPass(Args &...args) {
        [[maybe_unused]]
        bool result = (this->evaluate(args) & ...);
    }

  private:
    /* Rewrite an immediate with given constant. */
    static void rewrite(Immediate &data, target_size_t value) {
        data.data = std::make_unique <IntImmediate> (value);
    }

    /* Get the integer value of an immediate. */
    static target_size_t get_integer(Immediate &data) {
        return dynamic_cast <IntImmediate &> (*data.data).data;
    }

    /* Move out the integer value and transform. */
    template <typename _Fn>
    static Immediate move_integer(Immediate &data, _Fn &&fn) {
        auto &imm = dynamic_cast <IntImmediate &> (*data.data);
        imm.data = fn(imm.data);
        return std::move(data);
    }

    /* Evaluate a tree immediate. */
    static bool evaluate_tree(Immediate &data) {
        auto &tree = dynamic_cast <TreeImmediate &> (*data.data);

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
            if (!evaluate(sub) || !success) { success = false; continue; }
            // All evaluations are successful.
            auto value = get_integer(sub);
            switch (last_op) {
                case ADD: result += value; break;
                case SUB: result -= value; break;
                default: runtime_assert(false);
            } last_op = op;
        }

        runtime_assert(last_op == END);
        return success ? (rewrite(data, result), true) : false;
    }

    /* Whether the child can be rewritten into a integer. */
    static bool evaluate(Immediate &data) {
        auto &imm = *data.data;

        if (dynamic_cast <IntImmediate *> (&imm))
            return true;

        if (dynamic_cast <StrImmediate *> (&imm))
            return false;

        if (auto *relative = dynamic_cast <RelImmediate *> (&imm)) {
            switch (relative->operand) {
                using enum RelImmediate::Operand;
                case HI:
                    if (!evaluate(relative->imm)) return false;
                    data = move_integer(relative->imm, [](auto x) { return x >> 12; });
                    return true;
                case LO:
                    if (!evaluate(relative->imm)) return false;
                    data = move_integer(relative->imm, [](auto x) { return x & 0xFFF; });
                    return true;
                default:
                    return false;
            }
        }

        return evaluate_tree(data);
    }
};


} // namespace dark
