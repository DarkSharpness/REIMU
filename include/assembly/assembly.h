#pragma once
#include <declarations.h>
#include <vector>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include <span>

namespace dark {

struct Assembler {
    Section current_section { Section::UNKNOWN };
    struct LabelData {
        std::size_t line_number {};
        std::size_t data_index  {};
        std::string_view label_name;
        bool    global  {};
        Section section {};
    };

    std::unordered_map <std::string, LabelData> labels;
    std::vector <std::unique_ptr <Storage>> storages;
    std::vector <std::pair<std::size_t, Section>> sections;

    std::string         file_info;      // Debug information
    std::size_t         line_number;    // Debug information

    explicit Assembler(std::string_view);

    struct StorageSlice {
        std::span <std::unique_ptr <Storage>> slice;
        Section section;
    };
    auto split_by_section() -> std::vector <StorageSlice>;

  private:

    void set_section(Section);
    void add_label(std::string_view);
    void parse_line(std::string_view);
    void parse_command(std::string_view, std::string_view);
    void parse_storage(std::string_view, std::string_view);

    auto parse_storage_impl(std::string_view, std::string_view) -> std::string_view;
    void parse_command_impl(std::string_view, std::string_view);
    void debug(std::ostream &os);
};

} // namespace dark
