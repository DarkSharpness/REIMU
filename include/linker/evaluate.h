#pragma once
#include "linker.h"
#include <utility.h>
#include <storage.h>
#include <ustring.h>

namespace dark {

struct Evaluator {
  protected:
    using _Storage_t    = Linker::_Storage_t;
    using _Table_t      = Linker::_Symbol_Table_t;

  private:
    const _Table_t &global_table;   // Table of global symbols.
    const _Table_t *local_table;    // Table within a file.
    std::size_t position;

  public:
    /**
     * An evaluator which can evaluate the immediate values.
     * It need global and local table to query the symbol position.
     */
    explicit Evaluator(const _Table_t &global_table)
        : global_table(global_table), local_table() {}

  protected:

    void set_local(const _Table_t *local_table) {
        this->local_table = local_table;
    }

    void set_position(std::size_t position) {
        this->position = position;
    }

    std::size_t get_current_location() const { return this->position; }

    /**
     * Get the position of the symbol.
     * If the symbol is not found, it will panic.
     */
    auto get_symbol_position(std::string_view str) const {
        if (auto it = this->local_table->find(str); it != this->local_table->end())
            return it->second.query_position();
        if (auto it = this->global_table.find(str); it != this->global_table.end())
            return it->second.query_position();
        panic("Unknown symbol \"{}\"", str);
    }

    /* Evaluate the given immediate value. */
    target_size_t evaluate_tree(const TreeImmediate &tree) {
        using enum TreeImmediate::Operator;
        auto last_op = ADD;
        target_size_t result = 0;
        for (auto &[sub, op] : tree.data) {
            auto value = evaluate(*sub.data);
            switch (last_op) {
                case ADD: result += value; break;
                case SUB: result -= value; break;
                default: runtime_assert(false);
            } last_op = op;
        }
        runtime_assert(last_op == END);
        return result;
    }

    /* Evaluate the given immediate value. */
    target_size_t evaluate(const ImmediateBase &imm) {
        if (auto *integer = dynamic_cast <const IntImmediate *> (&imm))
            return integer->data;

        if (auto *symbol = dynamic_cast <const StrImmediate *> (&imm)) {
            auto [absolute, relative] =
                this->get_symbol_position(symbol->data.to_sv());
            return absolute + relative;
        }

        if (auto *relative = dynamic_cast <const RelImmediate *> (&imm))
            switch (relative->operand) {
                using enum RelImmediate::Operand;
                case HI: return evaluate(*relative->imm.data) >> 12;
                case LO: return evaluate(*relative->imm.data) & 0xFFF;
                case PCREL_HI:
                    return (evaluate(*relative->imm.data) - this->position) >> 12;
                case PCREL_LO:
                    return (evaluate(*relative->imm.data) - this->position) & 0xFFF;
                default: runtime_assert(false);
            }

        return this->evaluate_tree(dynamic_cast <const TreeImmediate &> (imm));
    }
};

} // namespace dark
