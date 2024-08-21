#include "simulation/debug.h"
#include "assembly/forward.h"
#include "assembly/frontend/token.h"
#include "assembly/immediate.h"
#include "assembly/storage/immediate.h"
#include "declarations.h"
#include "assembly/frontend/lexer.h"
#include "assembly/frontend/match.h"
#include "interpreter/memory.h"
#include "interpreter/register.h"
#include "libc/libc.h"
#include "linker/layout.h"
#include "riscv/command.h"
#include "riscv/register.h"
#include "utility/cast.h"
#include "utility/error.h"
#include "utility/hash.h"
#include "utility/misc.h"
#include <cctype>
#include <cstddef>
#include <fmtlib.h>
#include <iostream>
#include <optional>
#include <ostream>
#include <string_view>
#include <variant>

namespace dark {

struct InvalidFormat {};

struct ImmEvaluator {
public:
    explicit ImmEvaluator(const RegisterFile &rf, const MemoryLayout &layout)
        : rf(rf), layout(layout) {}

    auto operator()(const Immediate &imm) -> target_size_t {
        return this->evaluate(*imm.data);
    }

private:
    auto get_symbol_position(std::string_view name) -> target_size_t {
        if (name.starts_with('$')) {
            name.remove_prefix(1);
            if (name == "pc") return rf.get_pc();
            return rf[sv_to_reg(name)];
        }

        auto iter = layout.position_table.find(std::string(name));
        if (iter != layout.position_table.end())
            return iter->second;
        throw InvalidFormat {};
    }

    const RegisterFile &rf;
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

DebugManager::DebugManager(const RegisterFile &rf, const Memory &mem, const MemoryLayout &layout)
    : rf(rf), mem(mem), layout(layout) {
    for (auto &[label, pos] : layout.position_table)
        this->map.add(pos, label);
    this->call_stack.push_back({ layout.position_table.at("main"), rf.get_start_pc(), rf[Register::sp] });
}

auto DebugManager::pretty_address(target_size_t pc) -> std::string {
    if (pc > layout.bss.end()) // heap or stack, no label
        return std::format("{:#x}", pc);

    auto [label, offset] = this->map.get(pc);
    return std::format("{:#x} <{} + {}>", pc, label, offset);
}

auto DebugManager::has_breakpoint(target_size_t pc) const -> bool {
    return std::ranges::find(this->breakpoints, pc, &BreakPoint::pc) != this->breakpoints.end();
}

auto DebugManager::add_breakpoint(target_size_t pc) -> int {
    static int counter = 0;
    this->breakpoints.push_back({ pc, counter });
    return counter++;
}

/* Parse format such as 10i, f, 3w */
auto extract_unit(frontend::TokenStream &stream) {
    struct UnitPack {
        std::size_t cnt;
        char unit;
    };

    if (stream.empty()) throw InvalidFormat {};
    auto first = stream.split_at(1)[0];

    char suffix = '\0';
    if (!first.what.empty() && std::isalpha(first.what.back())) {
        suffix = first.what.back();
        first.what.remove_suffix(1);
    }

    std::size_t cnt = 1;
    if (!first.what.empty()) {
        auto value = sv_to_integer<std::size_t>(first.what);
        if (!value.has_value())
            throw InvalidFormat {};
        cnt = value.value();
    }

    return UnitPack { cnt, suffix };
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

    const auto __step = [&]() {
        auto [imm] = frontend::match <Immediate> (tokens);
        auto cnt = ImmEvaluator { this->rf, this->layout } (imm);
        this->option = Step { cnt };
        console::message << "Stepp " << cnt << " times" << std::endl;
        return true;
    };

    const auto __continue = [&]() {
        this->option = Continue {};
        return true;
    };

    const auto __breakpoint = [&]() {
        auto [imm] = frontend::match <Immediate> (tokens);
        auto pos = ImmEvaluator { this->rf, this->layout } (imm);
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
    };

    const auto __exhibit = [&]() {
        auto [cnt, suffix] = extract_unit(tokens);
        auto [imm] = frontend::match <Immediate> (tokens);
        auto pos = ImmEvaluator { this->rf, this->layout } (imm);
        const auto __print_data = [&](auto data) {
            if (pos % alignof(decltype(data)) != 0) {
                console::message << "Error: Data is not aligned" << std::endl;
                return false;
            }

            try {
                for (std::size_t i = 0; i < cnt; ++i) {
                    target_ssize_t data {};
                    target_ssize_t addr = pos + i * sizeof(data);

                    if constexpr (sizeof(data) == 4) {
                        data = const_cast<Memory &>(mem).load_i32(addr);
                    } else if constexpr (sizeof(data) == 2) {
                        data = const_cast<Memory &>(mem).load_i16(addr);
                    } else if constexpr (sizeof(data) == 1) {
                        data = const_cast<Memory &>(mem).load_i8(addr);
                    } else {
                        unreachable("Unsupported data size");
                    }

                    console::message
                        << std::format("{}\t {}", pretty_address(addr), data)
                        << std::endl;
                }
            } catch (...) {
                console::message << "Error: Data is out of range" << std::endl;
            }

            return false;
        };

        const auto __print_text = [&]() {
            if (pos % alignof(command_size_t) != 0) {
                console::message << "Error: Instruction is not aligned" << std::endl;
                return false;
            }

            static_assert(sizeof(command_size_t) == 4, "We assume the size of command is 4 bytes");
            if (pos < this->layout.text.begin() || pos + cnt * 4 > this->layout.text.end()) {
                console::message << "Error: Instruction is out of range" << std::endl;
                return false;
            }

            for (std::size_t i = 0; i < cnt; ++i) {
                auto cmd = const_cast<Memory &>(mem).load_cmd(pos + i * 4);
                console::message
                    << std::format("{}\t {}", pretty_address(pos + i * 4), pretty_command(cmd, pos + i * 4))
                    << std::endl;
            }
            return false;
        };

        if (suffix == 'i') {
            return __print_text();
        } else if (suffix == 'w') {
            return __print_data(std::int32_t {});
        } else if (suffix == 'h') {
            return __print_data(std::int16_t {});
        } else if (suffix == 'b') {
            return __print_data(std::int8_t {});
        } else {
            console::message << "Error: Invalid data type" << std::endl;
            return false;
        }
    };

    const auto __print = [&]() {
        auto [_, suffix] = extract_unit(tokens);
        auto [imm] = frontend::match <Immediate> (tokens);
        auto value = ImmEvaluator { this->rf, this->layout } (imm);
        if (suffix == 'x') {
            console::message << std::format("0x{:x}", value) << std::endl;
        } else if (suffix == 'd') {
            console::message << std::format("{}", value) << std::endl;
        } else if (suffix == 'c') {
            console::message << std::format("{}", static_cast<char>(value)) << std::endl;
        } else if (suffix == 't') {
            console::message << std::format("0b{:b}", value) << std::endl;
        } else if (suffix == 'i') {
            console::message << std::format("{}", pretty_command(value, 0)) << std::endl;
        } else if (suffix == 'a') {
            console::message << std::format("{}", pretty_address(value)) << std::endl;
        } else {
            console::message << "Error: Invalid print type" << std::endl;
        }
        return false;
    };

    using hash::switch_hash_impl;
    #define match_str(str) \
        case switch_hash_impl(str): \
            if (first.what != str) break;

    switch (switch_hash_impl(first.what)) {
        match_str("s")          return __step();
        match_str("step")       return __step();
        match_str("c")          return __continue();
        match_str("continue")   return __continue();
        match_str("b")          return __breakpoint();
        match_str("breakpoint") return __breakpoint();
        match_str("x")          return __exhibit();
        match_str("p")          return __print();
        match_str("print")      return __print();
        match_str("q")          return this->exit(), true;
        match_str("quit")       return this->exit(), true;

        default: break;
    }

    throw InvalidFormat {};
}

void DebugManager::terminal(target_size_t pc, command_size_t cmd)  {
    allow_unused(pc, cmd);
    this->option = std::monostate {};
    std::string str;

    console::message << "$ ";
    while (std::getline(std::cin, str)) {
        try {
            if (parse_line(str)) {
                console::message << std::endl;
                return;
            }
        } catch (...) {
            // Fall through
            console::message << "Invalid command format!" << std::endl;
        }
        console::message << "$ ";
    }

    return this->exit();
}

void DebugManager::test() {
    const auto pc = rf.get_pc();
    panic_if(pc % alignof(command_size_t) != 0, "Debugger: PC is not aligned");

    bool hit = false;

    if (this->has_breakpoint(pc)) {
        hit = true;
        console::message << std::format("Breakpoint hit at {}", pretty_address(pc)) << std::endl;
    }

    if (std::holds_alternative<Step>(this->option)) {
        auto &step = std::get<Step>(this->option);
        if (--step.count == 0) {
            this->option = std::monostate {};
            hit = true;
        }
    } else if (std::holds_alternative<std::monostate>(this->option)) {
        hit = true;
    }

    this->latest_pc.push_back(pc);

    constexpr auto kRet = []() {
        dark::command::jalr ret {};
        ret.rd = reg_to_int(Register::zero);
        ret.rs1 = reg_to_int(Register::ra);
        ret.imm = 0;
        return ret;
    } ();

    const auto cmd = pc < libc::kLibcEnd ? this->kEcall : const_cast<Memory &>(mem).load_cmd(pc);

    if (cmd == kRet.to_integer() || cmd == this->kEcall) {
        panic_if(this->call_stack.empty(), "Debugger: Call stack is empty");
        auto [_, caller_pc, caller_sp] = this->call_stack.back();
        this->call_stack.pop_back();
        panic_if(caller_pc + 4  != rf[Register::ra], "Debugger: Call stack is corrupted");
        panic_if(caller_sp      != rf[Register::sp], "Debugger: Stack pointer is corrupted");
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

void DebugManager::exit() {
    console::message << "Debugger exited" << std::endl;
    this->breakpoints = {};
    this->option = Continue {};
}


} // namespace dark
