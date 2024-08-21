#pragma once
#include "declarations.h"
#include "interpreter/memory.h"
#include "interpreter/register.h"
#include "libc/libc.h"
#include "linker/layout.h"
#include <map>
#include <string_view>
#include <variant>
#include <vector>

namespace dark {

struct LabelMap {
private:
    std::map <target_size_t, std::string_view> labels;

public:
    void add(target_size_t pc, std::string_view label) {
        labels[pc] = label;
    }

    auto map() const -> const decltype(labels) & {
        return labels;
    }

    auto get(target_size_t pc) const {
        struct Result {
            std::string_view label;
            target_size_t offset;
        };

        if (pc >= libc::kLibcStart && pc < libc::kLibcEnd) {
            auto which = (pc - libc::kLibcStart) / sizeof(command_size_t);
            auto name = libc::names[which];
            return Result { name, 0 };
        } else {
            auto pos = labels.upper_bound(pc);
            if (pos == labels.begin()) return Result { "", pc };
            --pos;
            return Result { pos->second, pc - pos->first };
        }
    }
};

struct DebugManager {
public:
    explicit DebugManager(const RegisterFile &rf, const Memory &mem, const MemoryLayout &layout);
    void test();

    auto get_rf() const -> const RegisterFile & { return this->rf; }
    auto pretty_address(target_size_t pc) -> std::string;
    auto pretty_command(command_size_t cmd, target_size_t pc) -> std::string;

private:
    static constexpr command_size_t kEcall = 0b1110011;

    struct Continue {};
    struct Step { target_size_t count; };
    struct Exit {};

    struct CallInfo {
        target_size_t callee_pc; // Function entry
        target_size_t caller_pc; // Where the function was called
        target_size_t caller_sp; // Stack pointer of the caller
    };

    struct BreakPoint {
        target_size_t pc;
        int index;
    };

    std::variant <std::monostate, Continue, Step> option;
    std::vector <target_size_t> latest_pc;      // Latest PC
    std::vector <CallInfo>      call_stack;     // Call Stack
    std::vector <BreakPoint>    breakpoints;    // Breakpoints

    LabelMap map;

    const RegisterFile  &rf;
    const Memory        &mem;
    const MemoryLayout  &layout;

    auto has_breakpoint(target_size_t pc) const -> bool;
    auto add_breakpoint(target_size_t pc) -> int;
    auto del_breakpoint(int index) -> bool;

    /* Builtin terminal for the debug console. */
    void terminal(target_size_t pc, command_size_t cmd);

    void exit();

    auto parse_line(std::string_view line) -> bool;
};


} // namespace dark