#include <utility.h>
#include <assembly/assembly.h>
#include <assembly/utility.h>
#include <assembly/parser.h>
#include <fstream>
#include <algorithm>

namespace dark {

Assembler::Assembler(std::string_view file_name)
: current_section(Section::UNKNOWN), file_name(file_name), line_number(0) {
    std::ifstream file { this->file_name };
    panic_if(!file.is_open(), "Failed to open {}", file_name);

    this->line_number = 0;
    std::string line;   // Current line

    while (std::getline(file, line)) {
        ++this->line_number;
        try {
            this->parse_line(line);
        } catch (FailToParse &e) {
            file.close();
            this->handle_at(this->line_number, std::move(e.inner));
        } catch(std::exception &e) {
            std::cerr << std::format("Unexpected error: {}\n", e.what());
            unreachable();
        } catch(...) {
            std::cerr << "Unexpected error.\n";
            unreachable();
        }
    }
}

void Assembler::set_section(Section section) {
    this->current_section = section;
    this->sections.emplace_back(this->storages.size(), section);
}

/**
 * @brief Parse a line of the assembly.
 * @param line Line of the assembly.
*/
void Assembler::parse_line(std::string_view line) {
    line = remove_front_whitespace(line);

    // Comment line case
    if (line.starts_with('#')) return;

    // Whether the line starts with a label
    if (auto label = start_with_label(line); label.has_value())
        return this->add_label(*label);

    // Not a label, find the first token
    auto [token, rest] = find_first_token(line);

    // Empty line case, nothing happens
    if (token.empty()) return;

    if (token[0] != '.') {
        return this->parse_command(token, rest);
    } else {
        token.remove_prefix(1);
        return this->parse_storage(token, rest);
    }
}

/**
 * @brief Try to add a label to the assembly.
 * @param label Label name.
 * @return Whether the label is successfully added.
 */
void Assembler::add_label(std::string_view label) {
    auto [iter, success] = this->labels.try_emplace(std::string(label));
    auto &label_info = iter->second;

    throw_if(success == false && label_info.is_defined(),
        "Label \"{}\" already exists\n"
        "First appearance at line {} in {}",
        label, label_info.line_number, this->file_name);

    throw_if(this->current_section == Section::UNKNOWN,
        "Label must be defined in a section");

    label_info.define_at(
        this->line_number, this->storages.size(),
        iter->first, this->current_section);
}

/**
 * @brief Parse the first token in the line.
 * @param token Initial token.
 * @param rest Rest of the line.
 * @return Whether the parsing is successful.
 */
void Assembler::parse_storage(std::string_view token, std::string_view rest) {
    constexpr auto __parse_section = [](std::string_view rest) -> std::optional <std::string_view> {
        rest = remove_front_whitespace(rest);
        if (!rest.empty() && rest.front() == '.') {
            rest.remove_prefix(1);
            if (match_prefix(rest, { "text" }))
                return "text";
            if (match_prefix(rest, { "data", "sdata" }))
                return "data";
            if (match_prefix(rest, { "bss", "sbss" }))
                return "bss";
            if (match_prefix(rest, { "rodata" }))
                return "rodata";
        }
        warning("Unknown section: {}", rest);
        return std::nullopt;
    };

    if (match_string(token, { "section" })) {
        if (auto result = __parse_section(rest); !result.has_value()) {
            return this->set_section(Section::UNKNOWN);
        } else {
            token = *result;
            rest = std::string_view {};
        }
    }

    auto new_rest = this->parse_storage_impl(token, rest);

    // Fail to parse the token
    if (!contain_no_token(new_rest)) throw FailToParse {};
}

auto Assembler::parse_storage_impl(std::string_view token, std::string_view rest) -> std::string_view {
    constexpr auto __set_section = [](Assembler *ptr, std::string_view rest, Section section) {
        ptr->set_section(section);
        return rest;
    };
    constexpr auto __set_globl = [](Assembler *ptr, std::string_view rest) {
        rest = remove_comments_when_no_string(rest);
        rest = remove_front_whitespace(rest);
        rest = remove_back_whitespace(rest);
        ptr->labels[std::string(rest)].set_global(ptr->line_number);
        return std::string_view {};
    };
    constexpr auto __set_align = [](Assembler *ptr, std::string_view rest) {
        constexpr std::size_t kMaxAlign = 20;
        auto [num_str, new_rest] = find_first_token(rest);
        auto num = sv_to_integer <std::size_t> (num_str).value_or(kMaxAlign);
        throw_if(num >= kMaxAlign, "Invalid alignment value: \"{}\"", rest);
        ptr->storages.push_back(std::make_unique <Alignment> (std::size_t{1} << num));
        return new_rest;
    };
    constexpr auto __set_bytes = [](Assembler *ptr, std::string_view rest, IntegerData::Type n) {
        auto [first, new_rest] = find_first_token(rest);
        ptr->storages.push_back(std::make_unique <IntegerData> (first, n));
        return new_rest;
    };
    constexpr auto __set_asciz = [](Assembler *ptr, std::string_view rest) {
        auto [str, new_rest] = find_first_asciz(rest);
        ptr->storages.push_back(std::make_unique <ASCIZ> (std::move(str)));
        return new_rest;
    };
    constexpr auto __set_zeros = [](Assembler *ptr, std::string_view rest) {
        auto [num_str, new_rest] = find_first_token(rest);
        constexpr std::size_t kMaxZeros = std::size_t{1} << 20;
        auto num = sv_to_integer <std::size_t> (num_str).value_or(kMaxZeros);
        throw_if(num >= kMaxZeros, "Invalid zero count: \"{}\"", num_str);
        ptr->storages.push_back(std::make_unique <ZeroBytes> (num));
        return new_rest;
    };
    constexpr auto __set_label = [](Assembler *ptr, std::string_view rest) {
        auto [label, new_rest] = find_first_token(rest);
        throw_if(!new_rest.starts_with(','));
        new_rest.remove_prefix(1);
        new_rest = remove_front_whitespace(new_rest);
        new_rest = remove_back_whitespace(new_rest);
        /* Allow new rest = . or . + 0 */
        if (new_rest == "." || new_rest == ". + 0") {
            ptr->add_label(label);
            return std::string_view {};
        }
        throw FailToParse {};
    };

    // Warn once for those known yet ignored attributes.
    constexpr auto __warn_once = [](std::string_view str) {
        static std::unordered_set <std::string> ignored_attributes;
        if (ignored_attributes.insert(std::string(str)).second)
            warning("attribute ignored: .{}", str);
        return std::string_view {};
    };

    using hash::switch_hash_impl;

    #define match_or_break(str, expr, ...) case switch_hash_impl(str):\
        if (token == str) { return expr(this, rest, ##__VA_ARGS__); } break

    switch (switch_hash_impl(token)) {
        // Data section
        match_or_break("data",      __set_section, Section::DATA);
        match_or_break("sdata",     __set_section, Section::DATA);
        match_or_break("bss",       __set_section, Section::BSS);
        match_or_break("sbss",      __set_section, Section::BSS);
        match_or_break("rodata",    __set_section, Section::RODATA);
        match_or_break("text",      __set_section, Section::TEXT);

        // Real storage
        match_or_break("align",     __set_align);
        match_or_break("p2align",   __set_align);
        match_or_break("byte",      __set_bytes, IntegerData::Type::BYTE);
        match_or_break("short",     __set_bytes, IntegerData::Type::SHORT);
        match_or_break("half",      __set_bytes, IntegerData::Type::SHORT);
        match_or_break("2byte",     __set_bytes, IntegerData::Type::SHORT);
        match_or_break("long",      __set_bytes, IntegerData::Type::LONG);
        match_or_break("word",      __set_bytes, IntegerData::Type::LONG);
        match_or_break("4byte",     __set_bytes, IntegerData::Type::LONG);
        match_or_break("string",    __set_asciz);
        match_or_break("asciz",     __set_asciz);
        match_or_break("zero",      __set_zeros);
        match_or_break("globl",     __set_globl);
        match_or_break("set",       __set_label);

        default: break;
    }
    #undef match_or_break
    return __warn_once(token);
}

} // namespace dark
