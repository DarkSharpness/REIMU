#include "simulation/debug.h"
#include "assembly/forward.h"
#include "assembly/frontend/token.h"
#include "assembly/immediate.h"
#include "assembly/storage/immediate.h"
#include "declarations.h"
#include "assembly/frontend/lexer.h"
#include "assembly/frontend/match.h"
#include "interpreter/exception.h"
#include "interpreter/memory.h"
#include "interpreter/register.h"
#include "libc/libc.h"
#include "linker/layout.h"
#include "riscv/command.h"
#include "riscv/register.h"
#include "utility/cast.h"
#include "utility/error.h"
#include "utility/hash.h"
#include "utility/ustring.h"
#include <cctype>
#include <concepts>
#include <cstddef>
#include <fmtlib.h>
#include <format>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <variant>
#include <ranges>

namespace dark {

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

        panic("Unknown symbol: {}", name);
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
            panic("Relative immediate is not supported in debug mode.");

        return this->evaluate_tree(dynamic_cast <const TreeImmediate &> (imm));
    }
};

/* Match an immediate number. */
static auto match_immediate(frontend::TokenStream &stream) {
    try {
        return match<Immediate>(stream);
    } catch (FailToParse &) {
        panic("Invalid immediate value");
    } catch (...) {
        panic("Unknown exception occurred");
    }
}

/* Parse format such as 10i, f, 3w */
static auto extract_unit(frontend::TokenStream &stream) {
    struct UnitPack {
        std::size_t cnt;
        char unit;
    };

    panic_if(stream.empty(), "Fail to parse the type");
    auto first = stream.split_at(1)[0];

    char suffix = '\0';
    if (!first.what.empty() && std::isalpha(first.what.back())) {
        suffix = first.what.back();
        first.what.remove_suffix(1);
    }

    std::size_t cnt = 1;
    if (!first.what.empty()) {
        auto value = sv_to_integer<std::size_t>(first.what);
        panic_if(!value.has_value(), "Invalid count");
        cnt = value.value();
    }

    return UnitPack { cnt, suffix };
}

/* Extract the only character in the stream. */
static auto extract_char(frontend::TokenStream &stream) -> char {
    panic_if(stream.empty(), "Fail to parse the type");
    auto first = stream.split_at(1)[0];
    panic_if(first.what.size() != 1, "Invalid type");
    return first.what[0];
}

static auto extract_int(frontend::TokenStream &stream) -> std::optional<std::optional<std::size_t>> {
    if (stream.empty()) return std::nullopt;
    auto first = stream.split_at(1)[0];
    return sv_to_integer<std::size_t>(first.what);
}

DebugManager::DebugManager(RegisterFile &rf, Memory &mem, Device &dev, const MemoryLayout &layout)
    : rf(rf), mem(mem), dev(dev), layout(layout), stack_range(mem.get_stack_start(), mem.get_stack_end()) {
    for (auto &[label, pos] : layout.position_table)
        this->map.add(pos, label);
    this->map.add(rf.get_start_pc(), "_start");
    this->map.add(mem.get_heap_start(), "_heap_start");
    // Note that we allow initial $sp not equal to _stack_start
    this->call_stack.push_back({ layout.position_table.at("main"), rf.get_start_pc(), rf[Register::sp] });
}

auto DebugManager::pretty_address(target_size_t pc) -> std::string {
    if (pc >= this->stack_range.first) {
        auto top = this->stack_range.second;
        auto offset = top - pc;
        return std::format("{:#x} <_stack_top - {}>", pc, offset);
    }

    auto [label, offset] = this->map.get(pc);
    return std::format("{:#x} <{} + {}>", pc, label, offset);
}

auto DebugManager::has_breakpoint(target_size_t pc) const -> bool {
    return std::ranges::find(this->breakpoints, pc, &BreakPoint::pc) != this->breakpoints.end();
}

auto DebugManager::del_breakpoint(int which) -> bool {
    auto iter = std::ranges::find(this->breakpoints, which, &BreakPoint::index);
    if (iter == this->breakpoints.end())
        return false;
    this->breakpoints.erase(iter);
    return true;
}

auto DebugManager::add_breakpoint(target_size_t pc) -> int {
    this->breakpoints.push_back({ pc, this->breakpoint_counter });
    return this->breakpoint_counter++;
}

auto DebugManager::del_display(int which) -> bool {
    auto iter = std::ranges::find(this->display_info, which, &DisplayInfo::index);
    if (iter == this->display_info.end())
        return false;
    this->display_info.erase(iter);
    return true;
}

auto DebugManager::add_display(DisplayInfo info, std::string_view name) -> int {
    const auto old_counter = this->display_counter;
    this->display_counter++;
    this->display_info.push_back(std::move(info));
    auto &last = this->display_info.back();
    last.index = old_counter;
    last.name  = unique_string(name);
    return old_counter;
}

void DebugManager::attach() {
    this->step_count += 1;
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


    constexpr auto kRet = []() {
        dark::command::jalr ret {};
        ret.rd = reg_to_int(Register::zero);
        ret.rs1 = reg_to_int(Register::ra);
        ret.imm = 0;
        return ret;
    } ();

    const auto cmd = this->fetch_cmd(pc);

    if (cmd == kRet.to_integer() || cmd == this->kEcall) {
        if (this->call_stack.empty()) {
            panic("Debugger: Call stack will be empty after this instruction");
        }

        auto [_, caller_pc, caller_sp] = this->call_stack.back();
        if (caller_pc + 4 != rf[Register::ra]) {
            panic(
                "Debugger: Call stack will be corrupted after this instruction\n"
                "\tOriginal ra: {:#x}\n"
                "\tCurrent  ra: {:#x}\n",
                caller_pc + 4, rf[Register::ra]
            );
        } else if (caller_sp != rf[Register::sp]) {
            panic(
                "Debugger: Stack pointer will be corrupted after this instruction\n"
                "\tOriginal sp: {:#x}\n"
                "\tCurrent  sp: {:#x}\n",
                caller_sp, rf[Register::sp]
            );
        }

        this->call_stack.pop_back();
    } else if ( // call instruction
        command::get_opcode(cmd) == command::jal::opcode
        && command::get_rd(cmd) == reg_to_int(Register::ra)) {
        auto call = dark::command::jal::from_integer(cmd);
        auto target_pc = pc + call.get_imm();
        this->call_stack.push_back({ target_pc, pc, rf[Register::sp] });
    }

    // Do not inline the terminal function
    if (hit) { [[unlikely]] this->terminal(); }

    this->latest_pc.push_back({ pc, cmd });
}

void DebugManager::exit() {
    console::message << "Debugger exited" << std::endl;
    this->breakpoints = {};
    this->option = Continue {};
}

auto DebugManager::fetch_cmd(target_size_t pc) -> command_size_t {
    return pc < libc::kLibcEnd ? this->kEcall : const_cast<Memory &>(mem).load_cmd(pc);
}

void DebugManager::terminal()  {
    this->option = std::monostate {};
    std::string str;

    const auto __show_terminal = [&]() {
        for (auto &info : this->display_info) {
            bool success = false;
            try {
                console::message
                << std::format("Display ${} | \"{}\"\n", info.index, info.name.to_sv())
                << std::endl;
                this->print_info_dispatch(info);
                success = true;
            } catch (FailToInterpret &e) {
                try { panic("{}", e.what(rf, mem, dev)); }
                catch (...) {}
            } catch (...) {}
            if (!success)
                console::message
                    << std::format("Error: Fail to display ${0:}. Try undisplay {0:}.\n", info.index);
            console::message << std::endl;
        }
        
        console::message << "\n$ ";
    };

    __show_terminal();

    while (std::getline(std::cin, str)) {
        // Print those display info
        try {
            this->terminal_cmds.push_back(str);
            if (parse_line(str)) {
                console::message << std::endl;
                return;
            }
        } catch (...) {
            console::message << "Invalid command format!" << std::endl;
        }
        __show_terminal();
    }

    return this->exit();
}

void DebugManager::print_info_dispatch(const DisplayInfo &info) {
    using console::message;

    static constexpr auto __print_data = [](DebugManager &d, auto data, std::size_t pos, std::size_t cnt) {
        panic_if(pos % alignof(decltype(data)) != 0,
            "Data is not aligned\n  Required alignment: {}", alignof(decltype(data)));

        for (std::size_t i = 0; i < cnt; ++i) {
            target_ssize_t data {};
            target_ssize_t addr = pos + i * sizeof(data);

            static_assert(std::integral<decltype(data)>);
            if constexpr (sizeof(data) == 4) {
                data = const_cast<Memory &>(d.mem).load_i32(addr);
            } else if constexpr (sizeof(data) == 2) {
                data = const_cast<Memory &>(d.mem).load_i16(addr);
            } else if constexpr (sizeof(data) == 1) {
                data = const_cast<Memory &>(d.mem).load_i8(addr);
            } else {
                unreachable("Unsupported data size");
            }

            message << std::format("{}\t {}", d.pretty_address(addr), data) << std::endl;
        }
    };

    static constexpr auto __print_text = [](DebugManager &d, std::size_t pos, std::size_t cnt) {
        panic_if(pos % alignof(command_size_t) != 0, "Instruction is not aligned");

        static_assert(sizeof(command_size_t) == 4, "We assume the size of command is 4 bytes");
        panic_if(pos < d.layout.text.begin() || pos + cnt * 4 > d.layout.text.end(), 
            "Instruction is out of range");

        for (std::size_t i = 0; i < cnt; ++i) {
            auto cmd = d.fetch_cmd(pos + i * 4);
            console::message
                << std::format("{}\t {}", d.pretty_address(pos + i * 4), d.pretty_command(cmd, pos + i * 4))
                << std::endl;
        }
    };

    static constexpr auto __display_mem = [](DebugManager &d, target_size_t value, const DisplayInfo &info) {
        const auto type = info.format;    // char
        const auto cnt  = info.count;     // std::size_t
        if (type == 'i') {
            return __print_text(d, value, cnt);
        } else if (type == 'w') {
            return __print_data(d, std::int32_t {}, value, cnt);
        } else if (type == 'h') {
            return __print_data(d, std::int16_t {}, value, cnt);
        } else if (type == 'b') {
            return __print_data(d, std::int8_t {}, value, cnt);
        } else {
            panic("Invalid memory type. Supported types: i, w, h, b");
        }
    };

    static constexpr auto __display_val = [](DebugManager &d, target_size_t value, const DisplayInfo &info) {
        const auto type = info.format;    // char

        if (type == 'x') {
            message << std::format("0x{:x}", value) << std::endl;
        } else if (type == 'd') {
            message << std::format("{}", static_cast<target_ssize_t>(value)) << std::endl;
        } else if (type == 'c') {
            message << std::format("{}", static_cast<char>(value)) << std::endl;
        } else if (type == 't') {
            message << std::format("0b{:b}", value) << std::endl;
        } else if (type == 'i') {
            message << std::format("{}", d.pretty_command(value, 0)) << std::endl;
        } else if (type == 'a') {
            message << std::format("{}", d.pretty_address(value)) << std::endl;
        } else {
            panic("Invalid value type. Supported types: x, d, c, t, i, a");
        }
    };

    /* Real implementation of the commands here.  */

    const auto value = ImmEvaluator { this->rf, this->layout } (info.imm);

    if (info.type == info.Memory) {
        __display_mem(*this, value, info);
    } else if (info.type == info.Value) {
        __display_val(*this, value, info);
    } else {
        unreachable("Invalid display type");
    }
}

auto DebugManager::parse_line(const std::string_view str) -> bool {
    frontend::Lexer lexer(str);
    auto tokens = lexer.get_stream();

    if (tokens.empty()) {
        this->terminal_cmds.pop_back();
        return false;
    }

    const auto first = tokens.front();
    if (first.type != frontend::Token::Type::Identifier) {
        this->terminal_cmds.pop_back();
        return false;
    }

    tokens = tokens.subspan(1);

    using console::message;

    /* Some helper functions.  */

    static constexpr auto __extract_mem = [](frontend::TokenStream &tokens) -> DisplayInfo {
        auto [cnt, suffix] = extract_unit(tokens);
        auto [imm] = match_immediate(tokens);
        return DisplayInfo {
            .imm    = std::move(imm),
            .count  = cnt,
            .format = suffix,
            .type   = DisplayInfo::Memory,
            .index  = {}, // Not used
            .name   = {}, // Not used
        };
    };

    static constexpr auto __extract_val = [](frontend::TokenStream &tokens) -> DisplayInfo {
        auto type = extract_char(tokens);
        auto [imm] = match_immediate(tokens);
        return DisplayInfo {
            .imm    = std::move(imm),
            .count  = {}, // Not used
            .format = type,
            .type   = DisplayInfo::Value,
            .index  = {}, // Not used
            .name   = {}, // Not used
        };
    };

    /* Real implementation of the commands here.  */

    const auto __step = [&]() {
        auto value = extract_int(tokens).value_or(1);
        panic_if(!value.has_value(), "Invalid step count");
        auto cnt = value.value();
        panic_if(cnt == 0, "Step count must be positive");
        this->option = Step { cnt };
        message << "Step " << cnt << " times" << std::endl;
        return true;
    };

    const auto __continue = [&]() {
        this->option = Continue {};
        return true;
    };

    const auto __breakpoint = [&]() {
        auto [imm] = match_immediate(tokens);
        auto pos = ImmEvaluator { this->rf, this->layout } (imm);
        if (this->has_breakpoint(pos)) {
            console::message
                << "Breakpoint already exists at " << pretty_address(pos)
                << std::endl;
        } else if (pos % alignof(command_size_t) != 0) {
            console::message
                << "Error: Breakpoint is not aligned to 4, which is unreachable"
                << std::endl;
        } else {
            auto which = this->add_breakpoint(pos);
            console::message
                << std::format("New breakpoint {} at {}", which, pretty_address(pos))
                << std::endl;
        }
        return false;
    };

    const auto __delete_bp = [&]() {
        auto value = extract_int(tokens).value_or(std::nullopt);
        panic_if(!value.has_value(), "Invalid breakpoint index");
        auto which = value.value();
        if (this->del_breakpoint(which)) {
            console::message
                << std::format("Breakpoint {} at {} is deleted", which, pretty_address(which))
                << std::endl;
        } else {
            console::message
                << std::format("Breakpoint {} does not exist", which)
                << std::endl;
        }
        return false;
    };

    const auto __info = [&]() {
        const std::string_view str = tokens.empty() ? "" : tokens[0].what;
        if (str == "breakpoint") {
            message << "Breakpoints:" << std::endl;
            for (auto &bp : this->breakpoints) {
                console::message
                    << std::format("  {} at {}", bp.index, pretty_address(bp.pc))
                    << std::endl;
            }
        } else if (str == "symbol") {
            message << "Symbols:" << std::endl;
            for (auto &[pos, name] : this->map.map()) {
                message << std::format("  {:<24} at {:#x}", name, pos) << std::endl;
            }
        } else if (str == "shell") {
            message << "History shell commands:" << std::endl;
            std::size_t i = 0;
            for (auto &cmd : this->terminal_cmds) {
                message << std::format("  {} | {}", i++, cmd) << std::endl;
            }
        } else if (str == "display") {
            message << "Displays:" << std::endl;
            for (auto &info : this->display_info) {
                console::message
                    << std::format("  {} | {}", info.index, info.name.to_sv())
                    << std::endl;
            }
        } else {
            panic("Invalid info type. Available types: breakpoint, symbol, shell");
        }
        return false;
    };

    const auto __exhibit = [&]() {
        this->print_info_dispatch(__extract_mem(tokens));
        return false;
    };

    const auto __print = [&]() {
        this->print_info_dispatch(__extract_val(tokens));
        return false;
    };

    const auto __backtrace = [&]() {
        message << "Backtrace:" << std::endl;
        for (auto [pc, caller_pc, caller_sp] : this->call_stack) {
            console::message
                << std::format("  {} called from {} with sp = {:#x}",
                    pretty_address(pc), pretty_address(caller_pc), caller_sp)
                << std::endl;
        }
        return false;
    };

    const auto __history = [&]() {
        message << "History:" << std::endl;
        auto value = extract_int(tokens).value_or(std::nullopt);
        panic_if(!value.has_value(), "Invalid history index");

        std::size_t cnt = std::min(value.value(), this->latest_pc.size());
        message << std::format("Last {} instructions:", cnt) << std::endl;
        for (auto [pc, cmd] : this->latest_pc | std::views::reverse | std::views::take(cnt)) {
            console::message
                << std::format("  {} {}", pretty_address(pc), pretty_command(cmd, pc))
                << std::endl;
        }

        return false;
    };

    const auto __exit = [&]() {
        this->exit();
        return true;
    };

    const auto __display = [&]() {
        // Display memory/value
        const auto type = extract_char(tokens);
        // Name = "display     m|v ......."
        const auto name = str.substr(8);
        if (type == 'm') {
            this->add_display(__extract_mem(tokens), name.substr(name.find('m')));
        } else if (type == 'v') {
            this->add_display(__extract_val(tokens), name.substr(name.find('v')));
        } else {
            panic("Error: Invalid display type. Supported types: m, v");
        }
        return false;
    };

    const auto __undisplay = [&]() {
        auto value = extract_int(tokens).value_or(std::nullopt);
        panic_if(!value.has_value(), "Invalid display index");
        auto which = value.value();
        if (this->del_display(which)) {
            console::message
                << std::format("Display {} is deleted", which)
                << std::endl;
        } else {
            console::message
                << std::format("Display {} does not exist", which)
                << std::endl;
        }
        return false;
    };

    const auto __help = [&]() {
        message <<
R"(Available commands:
    {s, step} [count]           Step [count] times
    {c, continue}               Continue
    {b, breakpoint} [address]   Add a breakpoint at [address]
    {d, delete} [index]         Delete the breakpoint with [index]
    {x} [count][type] [address] Exhibit [count] instructions or data at [address] 
    {p, print} [type] [address] Print the value at [address]
    {bt, backtrace}             Print the backtrace
    {i, info} [type]            Print the information of [type]
    {q, quit}                   Exit the debugger
    {h, history} [index]        Print the history of instructions
    {display} [type] [address]  Display the value at [address]
    {undisplay} [index]         Delete the display with [index]
    {help}                      Print this message
)" << std::endl;
        return false;
    };

    using hash::switch_hash_impl;
    #define match_str(str, func, msg) \
        case switch_hash_impl(str): \
            if (first.what != str) break; \
            try { \
                return func(); \
            } catch (FailToInterpret &e) { \
                try { panic("{}", e.what(rf, mem, dev)); } \
                catch (...) {} \
                message << msg << std::endl; \
                return false; \
            } catch (...) { \
                message << msg << std::endl; \
                return false; \
            }

    switch (switch_hash_impl(first.what)) {
        match_str("s",          __step, "Usage: s [count]")
        match_str("step",       __step, "Usage: step [count]")
        match_str("c",          __continue, "Usage: continue")
        match_str("continue",   __continue, "Usage: continue")
        match_str("b",          __breakpoint, "Usage: b [address]")
        match_str("breakpoint", __breakpoint, "Usage: breakpoint [address]")
        match_str("d",          __delete_bp, "Usage: d [index]")
        match_str("delete",     __delete_bp, "Usage: delete [index]")
        match_str("x",          __exhibit, "Usage: x [count][type] [address]")
        match_str("p",          __print, "Usage: p [type] [address]")
        match_str("print",      __print, "Usage: print [type] [address]")
        match_str("bt",         __backtrace, "Usage: bt")
        match_str("backtrace",  __backtrace, "Usage: backtrace")
        match_str("i",          __info, "Usage: i [type]")
        match_str("info",       __info, "Usage: info [type]")
        match_str("q",          __exit, "Usage: q")
        match_str("quit",       __exit, "Usage: quit")
        match_str("h",          __history, "Usage: h [index]")
        match_str("history",    __history, "Usage: history [index]")
        match_str("display",    __display, "Usage: display [m|v] [type] [address]")
        match_str("undisplay",  __undisplay, "Usage: undisplay [index]")
        match_str("help",       __help, "Usage: help")
        default: break;
    }

    try {
        panic("Unknown command: {}", first.what);
    } catch (...) {
        message << "use 'help' to see the list of available commands" << std::endl;
        return false;
    }
}

} // namespace dark
