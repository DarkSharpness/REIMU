#include "assembly/layout.h"
#include "config/config.h"
#include "interpreter/interpreter.h"
#include "linker/layout.h"
#include "linker/linker.h"
#include "utility/error.h"

namespace dark {

static void check_no_overlap(const MemoryLayout &result) {
    constexpr auto get_range = [](const auto &section) {
        return std::make_pair(section.begin(), section.end());
    };

    auto [text_start, text_end]     = get_range(result.text);
    auto [data_start, data_end]     = get_range(result.data);
    auto [rodata_start, rodata_end] = get_range(result.rodata);
    auto [bss_start, bss_end]       = get_range(result.bss);

    runtime_assert(text_end <= data_start && data_end <= rodata_start && rodata_end <= bss_start);
}

static void print_link_result(const Linker::LinkResult &result) {
    using dark::console::message;
    auto print_section = [](const std::string &name, const Linker::LinkResult::Section &section) {
        message << fmt::format(
            "Section {} \t at [{:#x}, {:#x})\n", name, section.start,
            section.start + section.storage.size()
        );
    };

    message << fmt::format("\n{:=^80}\n\n", " Section details ");

    print_section("text", result.text);
    print_section("data", result.data);
    print_section("rodata", result.rodata);
    print_section("bss", result.bss);

    message << fmt::format("\n{:=^80}\n\n", "");
}

void Interpreter::link() {
    auto &layouts = this->assembly_layout.get<std::vector<AssemblyLayout> &>();

    auto result = Linker{layouts}.get_linked_layout();

    panic_if(result.position_table.count("main") == 0, "No main function found");
    check_no_overlap(result);

    if (config.has_option("detail"))
        print_link_result(result);

    this->memory_layout = std::move(result);
}

} // namespace dark
