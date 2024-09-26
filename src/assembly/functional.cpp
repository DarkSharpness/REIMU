#include "assembly/assembly.h"
#include "assembly/exception.h"
#include "assembly/forward.h"
#include "assembly/layout.h"
#include "fmtlib.h"
#include "utility/error.h"
#include <algorithm>
#include <fstream>
#include <ranges>

namespace dark {

[[noreturn]]
auto handle_build_failure(std::string msg, const std::string &file_name, std::size_t line) -> void {
    using enum console::Color;
    using console::color_code;

    std::ifstream file{file_name};

    runtime_assert(line != 0 && file.is_open());

    const std::size_t windows[2] = {line - 1 ? line - 1 : 1, line + 1};

    auto counter  = windows[0];
    auto line_str = std::string{};
    while (--counter && std::getline(file, line_str))
        ;
    runtime_assert(counter == 0);
    std::string line_fmt_string;
    for (std::size_t i = windows[0]; i <= windows[1]; ++i) {
        if (!std::getline(file, line_str))
            break;
        if (i == line) {
            line_fmt_string +=
                fmt::format("{}{: >4}  |  {}{}\n", color_code<RED>, i, line_str, color_code<RESET>);
        } else {
            line_fmt_string += fmt::format("{: >4}  |  {}\n", i, line_str);
        }
    }

    if (msg.size() != 0 && msg.back() != '\n')
        msg.push_back('\n');

    if (line_fmt_string.ends_with('\n'))
        line_fmt_string.pop_back();

    panic(
        "{}Failure at {}{}:{}{}\n{}", msg, color_code<YELLOW>, file_name, line, color_code<RESET>,
        line_fmt_string
    );
}

void Assembler::debug(std::ostream &os) const {
    if (this->sections.empty())
        return;
    using _Pair_t = std::pair<std::size_t, const decltype(this->labels)::value_type *>;
    std::vector<_Pair_t> label_list;
    for (auto &pair : this->labels)
        label_list.emplace_back(pair.second.data_index, &pair);

    std::ranges::sort(label_list, {}, &_Pair_t::first);

    constexpr auto __print_section = [](std::ostream &os, auto *ptr, StorageSlice data,
                                        std::span<const _Pair_t> &view) {
        os << "    .section .";
        auto [slice, section] = data;
        switch (section) {
            case Section::TEXT:   os << "text\n"; break;
            case Section::DATA:   os << "data\n"; break;
            case Section::BSS:    os << "bss\n"; break;
            case Section::RODATA: os << "rodata\n"; break;
            default:              os << "unknown\n";
        }
        for (auto &storage : slice) {
            std::size_t which = &storage - ptr;
            while (!view.empty() && view.front().first == which) {
                auto [_, label_ptr] = view.front();
                view                = view.subspan(1);
                auto &[name, info]  = *label_ptr;
                if (info.global)
                    os << "    .globl " << name << '\n';
                os << name << ":\n";
            }
            storage->debug(os);
            os << '\n';
        }
    };

    std::span<const _Pair_t> label_view{label_list};

    for (auto part : this->split_by_section())
        __print_section(os, this->storages.data(), part, label_view);
}

auto Assembler::get_standard_layout() -> AssemblyLayout {
    AssemblyLayout layout;

    auto slices = this->split_by_section();

    layout.sections.reserve(slices.size());
    for (auto &[slice, section] : slices) {
        auto ptr = const_cast<std::unique_ptr<Storage> *>(slice.data());
        auto len = slice.size();
        layout.sections.push_back({std::span{ptr, len}, section});
    }

    const auto front_ptr = this->storages.data();
    layout.static_pool   = std::move(this->storages);
    // We utilize the fact that after moving a vector,
    // the inner pointer is still valid and points to the same memory.
    // auto *vec = std::any_cast <decltype(this->storages)>(&layout.static_storage);
    // runtime_assert(vec && vec->data() == front_ptr);
    runtime_assert(layout.static_pool.data() == front_ptr);

    layout.labels.reserve(this->labels.size());
    for (auto &label : this->labels | std::views::values) {
        auto &[line, index, name, global, section] = label;
        auto *storage                              = front_ptr + index;
        layout.labels.push_back({line, storage, std::string(name), global, section});
    }

    return layout;
}

auto Assembler::split_by_section() const -> std::vector<StorageSlice> {
    std::vector<StorageSlice> slices;
    if (this->sections.empty())
        return slices;

    auto storage              = std::span(this->storages);
    auto [prev, prev_section] = this->sections[0];
    for (std::size_t i = 1; i < this->sections.size(); ++i) {
        auto [next, next_section] = this->sections[i];
        auto prefix_size          = next - prev;
        slices.push_back({storage.subspan(0, prefix_size), prev_section});
        storage = storage.subspan(prefix_size);
        prev = next, prev_section = next_section;
    }

    slices.push_back({storage, prev_section});
    runtime_assert(prev_section == this->current_section);
    return slices;
}

void Assembler::set_section(Section section) {
    this->current_section = section;
    this->sections.emplace_back(this->storages.size(), section);
}

/**
 * @brief Try to add a label to the assembly.
 * @param label Label name.
 * @return Whether the label is successfully added.
 */
void Assembler::add_label(std::string_view label) {
    auto [iter, success] = this->labels.try_emplace(std::string(label));
    auto &label_info     = iter->second;

    throw_if(
        success == false && label_info.is_defined(),
        "Label \"{}\" already exists\n"
        "First appearance at line {} in {}",
        label, label_info.line_number, this->file_name
    );

    throw_if(this->current_section == Section::UNKNOWN, "Label must be defined in a section");

    label_info.define_at(
        this->line_number, this->storages.size(), iter->first, this->current_section
    );
}

} // namespace dark
