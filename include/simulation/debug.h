#pragma once
#include "assembly/storage/immediate.h"
#include "declarations.h"
#include "interpreter/memory.h"
#include "interpreter/register.h"
#include "libc/libc.h"
#include "linker/layout.h"
#include "riscv/register.h"
#include "utility/ustring.h"
#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace dark {

struct LabelMap {
private:
    std::map<target_size_t, std::string_view> labels;

public:
    void add(target_size_t pc, std::string_view label) { labels[pc] = label; }

    auto map() const -> const decltype(labels) & { return labels; }

    auto get(target_size_t pc) const {
        struct Result {
            std::string_view label;
            target_size_t offset;
        };

        if (pc >= libc::kLibcStart && pc < libc::kLibcEnd) {
            auto which = (pc - libc::kLibcStart) / sizeof(command_size_t);
            auto name  = libc::names[which];
            return Result{name, 0};
        } else {
            auto pos = labels.upper_bound(pc);
            if (pos == labels.begin())
                return Result{"", pc};
            --pos;
            return Result{pos->second, pc - pos->first};
        }
    }
};

struct DebugManager {
public:
    explicit DebugManager(RegisterFile &rf, Memory &mem, Device &dev, const MemoryLayout &layout);
    /* Attach the terminal before the command. */
    void attach();
    /* Builtin terminal for the debug console. */
    void terminal();

    auto get_rf() const -> const RegisterFile & { return this->rf; }
    auto pretty_address(target_size_t pc) -> std::string;
    auto pretty_command(command_size_t cmd, target_size_t pc) -> std::string;
    auto get_step() const -> std::size_t { return this->step_count; }

private:
    static constexpr command_size_t kEcall = 0b1110011;

    struct Halt {};
    struct Continue {};
    struct Step {
        std::size_t count;
    };
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

    struct History {
        target_size_t pc;
        command_size_t cmd;
    };

    struct DisplayInfo {
        Immediate imm;
        std::size_t count; // Count of the memory.
        char format;
        enum : bool {
            Memory,
            Value,
        } type;
        int index;
        unique_string name;
    };

    struct WatchInfo {
        // Memory or Register
        union {
            target_size_t addr; // Memory   op.
            Register reg;       // Register watch.
        };
        char format;
        enum : bool {
            Memory,
            Register_, // Conflict with dark::Register class name...
        } type;
        target_size_t init;
        int index;
    };

    std::variant<Halt, Continue, Step> option;
    std::vector<History> latest_pc;         // Latest PC
    std::vector<CallInfo> call_stack;       // Call Stack
    std::vector<BreakPoint> breakpoints;    // Breakpoints
    std::vector<DisplayInfo> display_info;  // Display information
    std::vector<WatchInfo> watch_info;      // Watch information
    std::vector<std::string> terminal_cmds; // Terminal commands

    LabelMap map;

    RegisterFile &rf;
    Memory &mem;
    Device &dev;
    const MemoryLayout &layout;

    std::size_t step_count = 0;
    const std::pair<target_size_t, target_size_t> stack_range;
    int breakpoint_counter = 0;
    int display_counter    = 0;
    int watch_counter      = 0;

    auto has_breakpoint(target_size_t pc) const -> bool;
    auto add_breakpoint(target_size_t pc) -> int;
    auto del_breakpoint(int index) -> bool;

    auto add_display(DisplayInfo info, std::string_view name) -> int;
    auto del_display(int index) -> bool;

    auto add_watch(WatchInfo info) -> int;
    auto del_watch(int index) -> bool;

    auto get_watch(const WatchInfo &info) const -> target_size_t;

    void exit();

    auto test_breakpoint(target_size_t pc) -> bool;
    auto test_action() -> bool;
    auto test_watch() -> bool;
    auto check_calling_convention(target_size_t pc) -> command_size_t;

    auto fetch_cmd(target_size_t pc) -> command_size_t;
    auto parse_line(const std::string_view line) -> bool;

    void print_info_dispatch(const DisplayInfo &info);
};


} // namespace dark
