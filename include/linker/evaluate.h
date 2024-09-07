#pragma once
#include "assembly/immediate.h"
#include "linker/linker.h"
#include "utility/cast.h"
#include "utility/error.h"

namespace dark {

struct Evaluator {
protected:
    using _Storage_t = Linker::_Storage_t;
    using _Table_t   = Linker::_Symbol_Table_t;

private:
    const _Table_t &global_table; // Table of global symbols.
    const _Table_t *local_table;  // Table within a file.
    target_size_t position;

protected:
    /**
     * An evaluator which can evaluate the immediate values.
     * It need global and local table to query the symbol position.
     */
    explicit Evaluator(const _Table_t &global_table) : global_table(global_table), local_table() {}

    void set_local(const _Table_t *local_table) { this->local_table = local_table; }

    void set_position(target_size_t position) { this->position = position; }

    void inc_position(target_size_t increment) { this->position += increment; }

    auto get_current_position() const { return this->position; }

    /**
     * Get the position of the symbol.
     * If the symbol is not found, it will panic.
     */
    auto get_symbol_position(std::string_view str) const {
        if (auto it = this->local_table->find(str); it != this->local_table->end())
            return it->second.get_location();
        if (auto it = this->global_table.find(str); it != this->global_table.end())
            return it->second.get_location();
        if (str == ".")
            return this->position;
        panic("Unknown symbol \"{}\"", str);
    }

    /* Evaluate the given immediate value. */
    auto evaluate_tree(const TreeImmediate &tree) -> target_size_t {
        using enum TreeImmediate::Operator;
        auto last_op = ADD;
        auto result  = target_size_t{};
        for (auto &[sub, op] : tree.data) {
            auto value = evaluate(*sub.data);
            switch (last_op) {
                case ADD: result += value; break;
                case SUB: result -= value; break;
                default:  unreachable();
            }
            last_op = op;
        }
        runtime_assert(last_op == END);
        return result;
    }

    /* Evaluate the given immediate value. */
    auto evaluate(const ImmediateBase &imm) -> target_size_t {
        if (auto *integer = dynamic_cast<const IntImmediate *>(&imm))
            return integer->data;

        if (auto *symbol = dynamic_cast<const StrImmediate *>(&imm))
            return this->get_symbol_position(symbol->data.to_sv());

        if (auto *relative = dynamic_cast<const RelImmediate *>(&imm))
            switch (auto value = evaluate(*relative->imm.data); relative->operand) {
                using enum RelImmediate::Operand;
                case HI:       return split_lo_hi(value).hi;
                case LO:       return split_lo_hi(value).lo;
                case PCREL_HI: return split_lo_hi(value - this->position).hi;
                case PCREL_LO: return split_lo_hi(value - this->position).lo;
                default:       unreachable();
            }

        return this->evaluate_tree(dynamic_cast<const TreeImmediate &>(imm));
    }
};

} // namespace dark
