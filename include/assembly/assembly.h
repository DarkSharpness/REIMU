#pragma once
#include "assembly/forward.h"
#include "declarations.h"
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

namespace dark {

struct Assembler {
    struct LabelData {
        std::size_t line_number{};
        std::size_t data_index{};
        std::string_view label_name;
        bool global{};
        Section section{};
        void
        define_at(std::size_t line, std::size_t index, const std::string &name, Section section) {
            this->line_number = line;
            this->data_index  = index;
            this->label_name  = name;
            this->section     = section;
        }
        void set_global(std::size_t line) {
            if (!this->is_defined())
                this->line_number = line;
            this->global = true;
        }
        bool is_defined() const { return !this->label_name.empty(); }
    };

    struct StorageSlice {
        std::span<const std::unique_ptr<Storage>> slice;
        Section section;
    };

    explicit Assembler(std::string_view);

    void debug(std::ostream &) const;

    /* Return the standard layout of a linker. */
    auto get_standard_layout() -> AssemblyLayout;

private:
    using Stream = frontend::TokenStream;

    Section current_section; // Current section

    std::unordered_map<std::string, LabelData> labels;
    std::vector<std::unique_ptr<Storage>> storages;
    std::vector<std::pair<std::size_t, Section>> sections;

    std::string file_name;   // Debug information
    std::size_t line_number; // Debug information

    void set_section(Section);
    void add_label(std::string_view);
    void parse_line(std::string_view);

    void parse_storage_new(std::string_view, const Stream &);
    void parse_command_new(std::string_view, const Stream &);

    [[noreturn]]
    void handle_at(std::size_t, std::string) const;

    auto split_by_section() const -> std::vector<StorageSlice>;

    template <typename _Tp, typename... _Args>
        requires std::constructible_from<_Tp, std::remove_reference_t<_Args>...>
    void push_new(_Args &&...args) {
        this->storages.push_back(std::make_unique<_Tp>(std::move(args)...));
    }
};

} // namespace dark
