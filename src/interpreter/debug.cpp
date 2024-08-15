#include "simulation/debug.h"
#include "assembly/frontend/token.h"
#include "assembly/immediate.h"
#include "assembly/storage/immediate.h"
#include "declarations.h"
#include "assembly/frontend/lexer.h"
#include "assembly/frontend/match.h"
#include "linker/layout.h"
#include "riscv/command.h"
#include "utility/error.h"
#include "utility/hash.h"
#include <fmtlib.h>
#include <iostream>
#include <string_view>

namespace dark {

struct InvalidFormat {};

struct ImmEvaluator {
public:
    explicit ImmEvaluator(const MemoryLayout &layout) : layout(layout) {}

    auto operator()(const Immediate &imm) -> target_size_t {
        return this->evaluate(*imm.data);
    }

private:
    auto get_symbol_position(std::string_view name) -> target_size_t {
        auto iter = layout.position_table.find(std::string(name));
        if (iter != layout.position_table.end())
            return iter->second;
        throw InvalidFormat {};
    }

    const MemoryLayout &layout;

    /* Evaluate the given immediate value. */
    auto evaluate_tree(const TreeImmediate &tree) -> target_size_t {
        using enum TreeImmediate::Operator;
        auto last_op = ADD;
        auto result  = target_size_t {};
        for (auto &[sub, op] : tree.data) {
            auto value = evaluate(*sub.data);
            switch (last_op) {
                case ADD: result += value; break;
                case SUB: result -= value; break;
                default: unreachable();
            } last_op = op;
        }
        runtime_assert(last_op == END);
        return result;
    }

    /* Evaluate the given immediate value. */
    auto evaluate(const ImmediateBase &imm) -> target_size_t {
        if (auto *integer = dynamic_cast <const IntImmediate *> (&imm))
            return integer->data;

        if (auto *symbol = dynamic_cast <const StrImmediate *> (&imm))
            return this->get_symbol_position(symbol->data.to_sv());

        if (dynamic_cast <const RelImmediate *> (&imm))
            throw InvalidFormat {};

        return this->evaluate_tree(dynamic_cast <const TreeImmediate &> (imm));
    }
};

auto DebugManager::pretty_address(target_size_t pc) -> std::string {
    auto [label, offset] = this->map.get(pc);
    return std::format("0x{:#x} <{} + {}>", pc, label, offset);
}

auto DebugManager::has_breakpoint(target_size_t pc) const -> bool {
    return std::ranges::find(this->breakpoints, pc) != this->breakpoints.end();
}

auto DebugManager::parse_line(std::string_view str) -> bool {
    frontend::Lexer lexer(str);
    auto tokens = lexer.get_stream();

    if (tokens.empty())
        return false;

    const auto first = tokens.front();
    if (first.type != frontend::Token::Type::Identifier)
        return false;

    tokens = tokens.subspan(1);

    using hash::switch_hash_impl;
    #define match_str(str) \
        case switch_hash_impl(str): \
            if (first.what != str) break;

    switch (switch_hash_impl(first.what)) { 
        match_str("s") { // Step
            auto [imm] = frontend::match <Immediate> (tokens);
            auto cnt = ImmEvaluator {layout} (imm);
            this->option = Step {cnt};
            console::message << "Step " << cnt << " times" << std::endl;
            return true;
        }

        match_str("c") { // Continue
            this->option = Continue {};
            return true;
        }

        match_str("b") {
            auto [imm] = frontend::match <Immediate> (tokens);
            auto pos = ImmEvaluator { this->layout } (imm);
            if (has_breakpoint(pos)) {
                console::message
                    << "Breakpoint already exists at " << pretty_address(pos)
                    << std::endl;
            } else {
                auto which = this->add_breakpoint(pos);
                console::message
                    << std::format("New breakpoint {} at {}", which, pretty_address(pos))
                    << std::endl;
            }
            return false;
        }

        default: break;
    }
    return false;
}

void DebugManager::terminal(target_size_t pc, command_size_t cmd)  {
    console::message
        << std::format("{} : {} ", pretty_address(pc), pretty_command(cmd))
        << std::endl;

    std::string str;
    while (std::getline(std::cin, str)) {
        try {
            if (parse_line(str)) break;
        } catch (...) {
            console::message << "Invalid command format!" << std::endl;
        }
    }
}

void DebugManager::test(const RegisterFile &rf, command_size_t cmd) {
    const auto pc = rf.get_pc();
    panic_if(pc % alignof(command_size_t) != 0, "Debugger: PC is not aligned");

    bool hit = false;
    if (std::ranges::find(this->breakpoints, pc) != this->breakpoints.end()) {
        hit = true;
        console::message << "Breakpoint hit at " << pc << std::endl;
    }

    if (std::holds_alternative<Step>(this->option)) {
        auto &step = std::get<Step>(this->option);
        if (step.count-- == 0) {
            this->option = Continue {};
            hit = true;
        }
    }

    this->latest_pc.push_back(pc);

    constexpr auto kRet = []() {
        dark::command::jalr ret {};
        ret.rd = reg_to_int(Register::zero);
        ret.rs1 = reg_to_int(Register::ra);
        return ret;
    } ();

    if (cmd == kRet.to_integer()) {
        panic_if(this->call_stack.empty(), "Debugger: Call stack is empty");
        auto [_, caller_pc, caller_sp] = this->call_stack.back();
        this->call_stack.pop_back();
        panic_if(caller_pc != rf[Register::ra], "Debugger: Call stack is corrupted");
        panic_if(caller_sp != rf[Register::sp], "Debugger: Stack pointer is corrupted");
    } else if ( // call instruction
        command::get_opcode(cmd) == command::jal::opcode
        && command::get_rd(cmd) == reg_to_int(Register::ra)) {
        auto call = dark::command::jal::from_integer(cmd);
        auto target_pc = pc + call.get_imm();
        this->call_stack.push_back({ target_pc, pc, rf[Register::sp] });
    }

    // Do not inline the terminal function
    if (hit) { [[unlikely]] this->terminal(pc, cmd); }
}


} // namespace dark
