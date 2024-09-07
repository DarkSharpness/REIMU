#include "declarations.h"
#include "fmtlib.h"
#include "interpreter/exception.h"
#include "interpreter/memory.h"
#include "interpreter/register.h"
#include "libc/libc.h"
#include "linker/layout.h"
#include "riscv/command.h"
#include "riscv/register.h"
#include "simulation/debug.h"
#include "utility/error.h"
#include "utility/ustring.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <ostream>
#include <string_view>
#include <variant>

namespace dark {

using console::message;

DebugManager::DebugManager(RegisterFile &rf, Memory &mem, Device &dev, const MemoryLayout &layout) :
    rf(rf), mem(mem), dev(dev), layout(layout),
    stack_range(mem.get_stack_start(), mem.get_stack_end()) {
    for (auto &[label, pos] : layout.position_table)
        this->map.add(pos, label);
    this->map.add(rf.get_start_pc(), "_start");
    this->map.add(mem.get_heap_start(), "_heap_start");
    this->map.add(stack_range.second, "_stack_top");
    // Note that we allow initial $sp not equal to _stack_start
    this->call_stack.push_back(
        {layout.position_table.at("main"), rf.get_start_pc(), rf[Register::sp]}
    );
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
    this->breakpoints.push_back({pc, this->breakpoint_counter});
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

    info.index = old_counter;
    info.name  = unique_string(name);
    this->display_info.push_back(std::move(info));

    return old_counter;
}

auto DebugManager::del_watch(int which) -> bool {
    auto iter = std::ranges::find(this->watch_info, which, &WatchInfo::index);
    if (iter == this->watch_info.end())
        return false;
    this->watch_info.erase(iter);
    return true;
}

auto DebugManager::add_watch(WatchInfo info) -> int {
    const auto old_counter = this->watch_counter;
    this->watch_counter++;

    info.index = old_counter;
    if (info.type == WatchInfo::Memory) {
        message << fmt::format("Watch memory at {}", pretty_address(info.addr)) << std::endl;
    } else if (info.type == WatchInfo::Register_) {
        if (info.reg == Register::zero) {
            console::message << "Don't be silly, you never change $zero" << std::endl;
            return -1;
        }
        message << fmt::format("Watch register ${}", reg_to_sv(info.reg)) << std::endl;
    } else {
        panic("Invalid watch type");
    }
    info.init = this->get_watch(info);

    this->watch_info.push_back(info);
    return old_counter;
}

auto DebugManager::get_watch(const WatchInfo &info) const -> target_size_t {
    if (info.type == WatchInfo::Register_)
        return this->rf[info.reg];
    // Must be memory.
    if (info.format == 'w') { // Most possible
        return this->mem.load_i32(info.addr);
    } else if (info.format == 'b') { // Next possible
        return this->mem.load_i8(info.addr);
    } else if (info.format == 'h') { // Why do so?
        return this->mem.load_i16(info.addr);
    } else {
        panic("Invalid memory type. Supported types: b, h, w");
    }
}

auto DebugManager::test_breakpoint(target_size_t pc) -> bool {
    if (this->has_breakpoint(pc)) {
        message << fmt::format("Breakpoint hit at {}", pretty_address(pc)) << std::endl;
        return true;
    } else {
        return false;
    }
}

auto DebugManager::test_watch() -> bool {
    bool modified = false;
    for (auto &info : this->watch_info) {
        auto current = this->get_watch(info);
        if (current != info.init) {
            message << fmt::format(
                           "Watch ${} is modified: {} -> {}", info.index, info.init, current
                       )
                    << std::endl;
            info.init = current;
            modified  = true;
        }
    }
    return modified;
}

auto DebugManager::test_action() -> bool {
    if (std::holds_alternative<Step>(this->option)) {
        auto &step = std::get<Step>(this->option);
        if (--step.count == 0) {
            this->option = Halt{};
            return true;
        }
    } else if (std::holds_alternative<Halt>(this->option)) {
        return true;
    }
    return false;
}

auto DebugManager::check_calling_convention(target_size_t pc) -> command_size_t {
    static constexpr auto kRet = []() {
        dark::command::jalr ret{};
        ret.rd  = reg_to_int(Register::zero);
        ret.rs1 = reg_to_int(Register::ra);
        ret.imm = 0;
        return ret;
    }();

    const auto cmd = this->fetch_cmd(pc);

    // return or ecall (which is also a return)
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
    } else if (command::get_rd(cmd) == reg_to_int(Register::ra)) {
        const auto opcode = command::get_opcode(cmd);
        if (opcode == command::jal::opcode) {
            auto call      = dark::command::jal::from_integer(cmd);
            auto target_pc = pc + call.get_imm();
            this->call_stack.push_back({target_pc, pc, rf[Register::sp]});
        } else if (opcode == command::jalr::opcode) {
            auto call      = dark::command::jalr::from_integer(cmd);
            auto target_pc = rf[int_to_reg(call.rs1)] + call.imm;
            this->call_stack.push_back({target_pc, pc, rf[Register::sp]});
        }
    }

    return cmd;
}

void DebugManager::attach() {
    this->step_count += 1;
    const auto pc = rf.get_pc();
    panic_if(pc % alignof(command_size_t) != 0, "Debugger: PC is not aligned");

    bool hit = false;

    hit |= this->test_breakpoint(pc);
    hit |= this->test_watch();
    hit |= this->test_action();

    const auto cmd = this->check_calling_convention(pc);

    // Do not inline the terminal function
    if (hit) {
        [[unlikely]] this->terminal();
    }

    this->latest_pc.push_back({pc, cmd});
}

void DebugManager::exit() {
    message << "Debugger exited" << std::endl;
    this->breakpoints = {};
    this->watch_info  = {};
    this->option      = Continue{};
}

auto DebugManager::fetch_cmd(target_size_t pc) -> command_size_t {
    return pc < libc::kLibcEnd ? this->kEcall : mem.load_cmd(pc);
}

void DebugManager::terminal() {
    this->option = Halt{};
    std::string str;

    const auto __show_terminal = []() { message << "\n$ "; };

    for (auto &info : this->display_info) {
        bool success = false;
        try {
            message << fmt::format("Display ${} | \"{}\"\n", info.index, info.name.to_sv())
                    << std::endl;
            this->print_info_dispatch(info);
            success = true;
        } catch (FailToInterpret &e) {
            try {
                panic("{}", e.what(rf, mem, dev));
            } catch (...) {}
        } catch (...) {}
        if (!success)
            message << fmt::format(
                "Error: Fail to display ${0:}. Try undisplay {0:}.\n", info.index
            );
        message << std::endl;
    }

    __show_terminal();

    while (std::getline(std::cin, str)) {
        try {
            this->terminal_cmds.push_back(str);
            if (parse_line(str)) {
                message << std::endl;
                return;
            }
        } catch (...) { message << "Invalid command format! Try 'help'" << std::endl; }
        __show_terminal();
    }

    return this->exit();
}

} // namespace dark
