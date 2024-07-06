#include <utility.h>
#include <config/config.h>
#include <interpreter/interpreter.h>
#include <assembly/assembly.h>
#include <linker/linker.h>
#include <libc/libc.h>

namespace dark {

static void check_no_overlap(const MemoryLayout &result) {
    constexpr auto get_range = [](const auto &section) {
        return std::make_pair(section.begin(), section.end());
    };

    auto [text_start, text_end] = get_range(result.text);
    auto [data_start, data_end] = get_range(result.data);
    auto [rodata_start, rodata_end] = get_range(result.rodata);
    auto [bss_start, bss_end] = get_range(result.bss);

    runtime_assert(
        text_end <= data_start &&
        data_end <= rodata_start &&
        rodata_end <= bss_start);
}

static void print_link_result(const Linker::LinkResult &result) {
    auto print_section = [](const std::string &name, const Linker::LinkResult::Section &section) {
        std::cout << std::format("Section {} \t at [{:x}, {:x})\n",
            name, section.start, section.start + section.storage.size());
    };

    std::cout << std::format("{:=^80}\n", " Section details ");

    print_section("text", result.text);
    print_section("data", result.data);
    print_section("rodata", result.rodata);
    print_section("bss", result.bss);

    std::cout << std::format("{:=^80}\n", "");
}

Interpreter::Interpreter(const Config &config) {
    std::vector <Assembler> assemblies;

    assemblies.reserve(config.get_assembly_names().size());
    for (const auto &file : config.get_assembly_names())
        assemblies.emplace_back(file);

    auto result = Linker{ assemblies }.get_result();
    panic_if(result.position_table.count("main") == 0, "No main function found");
    check_no_overlap(result);

    if (config.has_option("detail")) print_link_result(result);

    // Reset the memory storage
    assemblies = decltype(assemblies) {};

    this->layout = std::move(result);
}

} // namespace dark
