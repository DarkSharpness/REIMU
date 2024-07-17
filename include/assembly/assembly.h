#pragma once
#include <declarations.h>
#include <vector>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include <span>
#include <ranges>

namespace dark {

struct Storage;
struct AssemblyLayout;

struct Assembler {
    struct LabelData {
        std::size_t line_number {};
        std::size_t data_index  {};
        std::string_view label_name;
        bool    global  {};
        Section section {};
        void define_at(std::size_t line, std::size_t index,
            const std::string &name, Section section) {
            this->line_number = line;
            this->data_index = index;
            this->label_name = name;
            this->section = section;
        }
        void set_global(std::size_t line) {
            if (!this->is_defined())
                this->line_number = line;
            this->global = true;
        }
        bool is_defined() const { return !this->label_name.empty(); }
    };

    struct StorageSlice {
        std::span <const std::unique_ptr <Storage>> slice;
        Section section;
    };

    explicit Assembler(std::string_view);

    void debug(std::ostream &) const;

    /* Return the standard layout of a linker. */
    auto get_standard_layout() -> AssemblyLayout;

  private:
    Section current_section;    // Current section

    std::unordered_map <std::string, LabelData> labels;
    std::vector <std::unique_ptr <Storage>> storages;
    std::vector <std::pair<std::size_t, Section>> sections;

    std::string file_name;      // Debug information
    std::size_t line_number;    // Debug information

    void set_section(Section);
    void add_label(std::string_view);
    void parse_line(std::string_view);
    void parse_command(std::string_view, std::string_view);
    void parse_storage(std::string_view, std::string_view);

    auto parse_storage_impl(std::string_view, std::string_view) -> std::string_view;
    void parse_command_impl(std::string_view, std::string_view);

    [[noreturn]]
    void handle_at(std::size_t, std::string) const;

    auto split_by_section() const -> std::vector <StorageSlice>;

    template <typename _Tp, typename ..._Args>
    requires std::constructible_from <_Tp, std::remove_reference_t <_Args>...>
    void push_cmd(_Args &&...args);
};

} // namespace dark
