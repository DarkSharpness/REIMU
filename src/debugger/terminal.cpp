#include "assembly/exception.h"
#include "assembly/frontend/lexer.h"
#include "assembly/frontend/match.h"
#include "assembly/frontend/token.h"
#include "assembly/immediate.h"
#include "assembly/storage/immediate.h"
#include "fmtlib.h"
#include "interpreter/exception.h"
#include "simulation/debug.h"
#include "utility/cast.h"
#include "utility/error.h"
#include "utility/hash.h"
#include <cstddef>
#include <iostream>
#include <ranges>

namespace dark {

using console::message;

struct ImmEvaluator {
public:
    explicit ImmEvaluator(const RegisterFile &rf, const MemoryLayout &layout) :
        rf(rf), layout(layout) {}

    auto operator()(const Immediate &imm) -> target_size_t {
        try {
            return this->evaluate(*imm.data);
        } catch (FailToParse &e) { panic("{}", e.inner); }
    }

private:
    auto get_symbol_position(std::string_view name) -> target_size_t {
        if (name.starts_with('$')) {
            name.remove_prefix(1);
            if (name == "pc")
                return rf.get_pc();
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

        if (dynamic_cast<const RelImmediate *>(&imm))
            panic("Relative immediate is not supported in debug mode.");

        return this->evaluate_tree(dynamic_cast<const TreeImmediate &>(imm));
    }
};

void DebugManager::print_info_dispatch(const DisplayInfo &info) {
    static constexpr auto __print_data = [](DebugManager &d, auto data, std::size_t pos,
                                            std::size_t cnt) {
        panic_if(
            pos % alignof(decltype(data)) != 0, "Data is not aligned\n  Required alignment: {}",
            alignof(decltype(data))
        );

        for (std::size_t i = 0; i < cnt; ++i) {
            target_ssize_t data{};
            target_ssize_t addr = pos + i * sizeof(data);

            static_assert(std::integral<decltype(data)>);
            if constexpr (sizeof(data) == 4) {
                data = d.mem.load_i32(addr);
            } else if constexpr (sizeof(data) == 2) {
                data = d.mem.load_i16(addr);
            } else if constexpr (sizeof(data) == 1) {
                data = d.mem.load_i8(addr);
            } else {
                unreachable("Unsupported data size");
            }

            message << fmt::format("{}\t {}", d.pretty_address(addr), data) << std::endl;
        }
    };

    static constexpr auto __print_text = [](DebugManager &d, std::size_t pos, std::size_t cnt) {
        panic_if(pos % alignof(command_size_t) != 0, "Instruction is not aligned");

        static_assert(sizeof(command_size_t) == 4, "We assume the size of command is 4 bytes");
        panic_if(
            pos < d.layout.text.begin() || pos + cnt * 4 > d.layout.text.end(),
            "Instruction is out of range"
        );

        for (std::size_t i = 0; i < cnt; ++i) {
            auto cmd = d.fetch_cmd(pos + i * 4);
            message << fmt::format(
                           "{}\t {}", d.pretty_address(pos + i * 4),
                           d.pretty_command(cmd, pos + i * 4)
                       )
                    << std::endl;
        }
    };

    static constexpr auto __display_mem = [](DebugManager &d, target_size_t value,
                                             const DisplayInfo &info) {
        const auto type = info.format; // char
        const auto cnt  = info.count;  // std::size_t
        if (type == 'i') {
            return __print_text(d, value, cnt);
        } else if (type == 'w') {
            return __print_data(d, std::int32_t{}, value, cnt);
        } else if (type == 'h') {
            return __print_data(d, std::int16_t{}, value, cnt);
        } else if (type == 'b') {
            return __print_data(d, std::int8_t{}, value, cnt);
        } else {
            panic("Invalid memory type. Supported types: i, w, h, b");
        }
    };

    static constexpr auto __display_val = [](DebugManager &d, target_size_t value,
                                             const DisplayInfo &info) {
        const auto type = info.format; // char

        if (type == 'x') {
            message << fmt::format("0x{:x}", value) << std::endl;
        } else if (type == 'd') {
            message << fmt::format("{}", static_cast<target_ssize_t>(value)) << std::endl;
        } else if (type == 'c') {
            message << fmt::format("{}", static_cast<char>(value)) << std::endl;
        } else if (type == 't') {
            message << fmt::format("0b{:b}", value) << std::endl;
        } else if (type == 'i') {
            message << fmt::format("{}", d.pretty_command(value, 0)) << std::endl;
        } else if (type == 'a') {
            message << fmt::format("{}", d.pretty_address(value)) << std::endl;
        } else {
            panic("Invalid value type. Supported types: x, d, c, t, i, a");
        }
    };

    /* Real implementation of the commands here.  */

    const auto value = ImmEvaluator{this->rf, this->layout}(info.imm);

    if (info.type == info.Memory) {
        __display_mem(*this, value, info);
    } else if (info.type == info.Value) {
        __display_val(*this, value, info);
    } else {
        unreachable("Invalid display type");
    }
}

/* Match an immediate number. */
static auto match_immediate(frontend::TokenStream &stream) {
    try {
        return frontend::match<Immediate>(stream);
    } catch (FailToParse &) { panic("Invalid immediate value"); } catch (...) {
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

    return UnitPack{cnt, suffix};
}

/* Extract the only character in the stream. */
static auto extract_char(frontend::TokenStream &stream) -> char {
    panic_if(stream.empty(), "Fail to parse the type");
    auto first = stream.split_at(1)[0];
    panic_if(first.what.size() != 1, "Invalid type");
    return first.what[0];
}

/* Extract the string in the stream. */
static auto extract_register(frontend::TokenStream &stream) -> Register {
    panic_if(stream.empty(), "Fail to parse the register");
    auto first = stream.split_at(1)[0];
    panic_if(!first.what.starts_with('$'), "Invalid register");
    auto reg = sv_to_reg_nothrow(first.what.substr(1));
    panic_if(!reg.has_value(), "Invalid register");
    return reg.value();
}

static auto extract_int(frontend::TokenStream &stream)
    -> std::optional<std::optional<std::size_t>> {
    if (stream.empty())
        return std::nullopt;
    auto first = stream.split_at(1)[0];
    return sv_to_integer<std::size_t>(first.what);
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

    /* Some helper functions.  */

    static constexpr auto __extract_mem = [](frontend::TokenStream &tokens) -> DisplayInfo {
        auto [cnt, suffix] = extract_unit(tokens);
        auto [imm]         = match_immediate(tokens);
        return DisplayInfo{
            .imm    = std::move(imm),
            .count  = cnt,
            .format = suffix,
            .type   = DisplayInfo::Memory,
            .index  = {}, // Not used
            .name   = {}, // Not used
        };
    };

    static constexpr auto __extract_val = [](frontend::TokenStream &tokens) -> DisplayInfo {
        auto type  = extract_char(tokens);
        auto [imm] = match_immediate(tokens);
        return DisplayInfo{
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
        this->option = Step{cnt};
        message << "Step " << cnt << " times" << std::endl;
        return true;
    };

    const auto __continue = [&]() {
        this->option = Continue{};
        return true;
    };

    const auto __breakpoint = [&]() {
        auto [imm] = match_immediate(tokens);
        auto pos   = ImmEvaluator{this->rf, this->layout}(imm);
        if (this->has_breakpoint(pos)) {
            message << "Breakpoint already exists at " << pretty_address(pos) << std::endl;
        } else if (pos % alignof(command_size_t) != 0) {
            message << "Error: Breakpoint is not aligned to 4, which is unreachable" << std::endl;
        } else {
            auto which = this->add_breakpoint(pos);
            message << fmt::format("New breakpoint {} at {}", which, pretty_address(pos))
                    << std::endl;
        }
        return false;
    };

    const auto __delete_bp = [&]() {
        auto value = extract_int(tokens).value_or(std::nullopt);
        panic_if(!value.has_value(), "Invalid breakpoint index");
        auto which = value.value();
        if (this->del_breakpoint(which)) {
            message << fmt::format("Breakpoint {} at {} is deleted", which, pretty_address(which))
                    << std::endl;
        } else {
            message << fmt::format("Breakpoint {} does not exist", which) << std::endl;
        }
        return false;
    };

    const auto __info = [&]() {
        const std::string_view str = tokens.empty() ? "" : tokens[0].what;
        if (str == "breakpoint") {
            message << "Breakpoints:" << std::endl;
            for (auto &bp : this->breakpoints) {
                message << fmt::format("  {} at {}", bp.index, pretty_address(bp.pc)) << std::endl;
            }
        } else if (str == "symbol") {
            message << "Symbols:" << std::endl;
            for (auto &[pos, name] : this->map.map()) {
                message << fmt::format("  {:<24} at {:#x}", name, pos) << std::endl;
            }
        } else if (str == "shell") {
            message << "History shell commands:" << std::endl;
            std::size_t i = 0;
            for (auto &cmd : this->terminal_cmds) {
                message << fmt::format("  {} | {}", i++, cmd) << std::endl;
            }
        } else if (str == "display") {
            message << "Displays:" << std::endl;
            for (auto &info : this->display_info) {
                message << fmt::format("  {} | {}", info.index, info.name.to_sv()) << std::endl;
            }
        } else if (str == "watch") {
            message << "Watches:" << std::endl;
            for (auto &info : this->watch_info) {
                if (info.type == WatchInfo::Memory) {
                    message << fmt::format(
                        "  {} | Memory at {}", info.index, pretty_address(info.addr)
                    );
                } else if (info.type == WatchInfo::Register_) {
                    message << fmt::format("  {} | Register ${}", info.index, reg_to_sv(info.reg));
                } else {
                    panic("Invalid watch type");
                }
                message << std::endl;
            }
        } else {
            panic(
                "Invalid info type.\n  Available types: breakpoint, symbol, shell, display, watch"
            );
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
            message << fmt::format(
                           "  {} called from {} with sp = {:#x}", pretty_address(pc),
                           pretty_address(caller_pc), caller_sp
                       )
                    << std::endl;
        }
        return false;
    };

    const auto __history = [&]() {
        message << "History:" << std::endl;
        auto value = extract_int(tokens).value_or(std::nullopt);
        panic_if(!value.has_value(), "Invalid history index");

        std::size_t cnt = std::min(value.value(), this->latest_pc.size());
        message << fmt::format("Last {} instructions:", cnt) << std::endl;
        std::size_t counter = this->latest_pc.size();
        for (auto [pc, cmd] : this->latest_pc | std::views::reverse | std::views::take(cnt)) {
            message << fmt::format(
                           "{} | {} {}", counter, pretty_address(pc), pretty_command(cmd, pc)
                       )
                    << std::endl;
            counter--;
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
            message << fmt::format("Display {} is deleted", which) << std::endl;
        } else {
            message << fmt::format("Display {} does not exist", which) << std::endl;
        }
        return false;
    };

    const auto __watch = [&]() {
        // Watch some memory/value
        const auto type = extract_char(tokens);
        if (type == 'm') {
            // Watch memory
            auto info = __extract_mem(tokens);
            this->add_watch({
                .addr   = ImmEvaluator{this->rf, this->layout}(info.imm),
                .format = info.format,
                .type   = WatchInfo::Memory,
                .init   = {}, // Not used
                .index  = {}, // Not used
            });
        } else if (type == 'r') {
            // Watch Register
            this->add_watch({
                .reg    = extract_register(tokens),
                .format = {}, // Not used
                .type   = WatchInfo::Register_,
                .init   = {}, // Not used
                .index  = {}, // Not used
            });
        } else {
            panic("Error: Invalid watch type. Supported types: m, r");
        }

        return false;
    };

    const auto __unwatch = [&]() {
        auto value = extract_int(tokens).value_or(std::nullopt);
        panic_if(!value.has_value(), "Invalid watch index");
        auto which = value.value();
        if (this->del_watch(which)) {
            message << fmt::format("Watch {} is deleted", which) << std::endl;
        } else {
            message << fmt::format("Watch {} does not exist", which) << std::endl;
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
    {w, watch} [type] [address] Watch the value at [address]
    {unwatch} [index]           Delete the watch with [index]
    {help}                      Print this message
)" << std::endl;
        return false;
    };

    using hash::switch_hash_impl;
#define match_str(str, func, msg)                                                                  \
    case switch_hash_impl(str):                                                                    \
        if (first.what != str)                                                                     \
            break;                                                                                 \
        try {                                                                                      \
            return func();                                                                         \
        } catch (FailToInterpret & e) {                                                            \
            try {                                                                                  \
                panic("{}", e.what(rf, mem, dev));                                                 \
            } catch (...) {}                                                                       \
            message << msg << std::endl;                                                           \
            return false;                                                                          \
        } catch (...) {                                                                            \
            message << msg << std::endl;                                                           \
            return false;                                                                          \
        }

    switch (switch_hash_impl(first.what)) {
        match_str("s", __step, "Usage: s [count]") match_str(
            "step", __step, "Usage: step [count]"
        ) match_str("c", __continue, "Usage: continue")
            match_str("continue", __continue, "Usage: continue") match_str(
                "b", __breakpoint, "Usage: b [address]"
            ) match_str("breakpoint", __breakpoint, "Usage: breakpoint [address]")
                match_str("d", __delete_bp, "Usage: d [index]") match_str(
                    "delete", __delete_bp, "Usage: delete [index]"
                ) match_str("x", __exhibit, "Usage: x [count][type] [address]")
                    match_str("p", __print, "Usage: p [type] [address]") match_str(
                        "print", __print, "Usage: print [type] [address]"
                    ) match_str("bt", __backtrace, "Usage: bt")
                        match_str("backtrace", __backtrace, "Usage: backtrace") match_str(
                            "i", __info, "Usage: i [type]"
                        ) match_str("info", __info, "Usage: info [type]")
                            match_str("q", __exit, "Usage: q") match_str(
                                "quit", __exit, "Usage: quit"
                            ) match_str("h", __history, "Usage: h [index]")
                                match_str("history", __history, "Usage: history [index]") match_str(
                                    "display", __display, "Usage: display [m|v] [type] [address]"
                                ) match_str("undisplay", __undisplay, "Usage: undisplay [index]")
                                    match_str("w", __watch, "Usage: w [m|r] [type] [address]")
                                        match_str(
                                            "watch", __watch, "Usage: watch [m|r] [type] [address]"
                                        ) match_str("unwatch", __unwatch, "Usage: unwatch [index]")
                                            match_str("help", __help, "Usage: help") default
            : break;
    }

    try {
        panic("Unknown command: {}", first.what);
    } catch (...) {
        message << "use 'help' to see the list of available commands" << std::endl;
        return false;
    }
}

} // namespace dark
