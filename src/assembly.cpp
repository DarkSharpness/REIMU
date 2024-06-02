#include "utility.h"
#include "assembly.h"
#include <fstream>
#include <algorithm>

namespace dark {

struct FailToParse {
    std::string inner;
};

/** Just a code to indicate the error. */
struct ErrorCase {};

struct Alignment : public Assembly::Storage {
    std::size_t alignment;
    explicit Alignment(std::size_t pow) {}
    ~Alignment() override = default;
};

struct IntegerData : public Assembly::Storage {
    std::string data;
    enum class Type {
        // 1 << 0, 1 << 1, 1 << 2 Bytes
        BYTE = 0, SHORT = 1, LONG = 2
    } type;

    template <std::size_t _N>
    IntegerData(std::string_view data) : data(data) {
        if constexpr (_N == 1) this->type = Type::BYTE;
        else if constexpr (_N == 2) this->type = Type::SHORT;
        else if constexpr (_N == 4) this->type = Type::LONG;
        else static_assert(sizeof(_N) == 0, "Invalid size");
    }


    ~IntegerData() override = default;
};

struct ASCIZ : public Assembly::Storage {
    std::string data;
    explicit ASCIZ(std::string_view str) {}
    ~ASCIZ() override = default;
};

/* Remove the front whitespace of the string. */
static auto remove_front_whitespace(std::string_view str) -> std::string_view {
    while (!str.empty() && std::isspace(str.front()))
        str.remove_prefix(1);
    return str;
}

/* Whether the string part contains no token. */
static bool have_no_token(std::string_view str) {
    std::string_view tmp = remove_front_whitespace(str);
    return tmp.empty() || tmp.front() == '#';
}

/* Whether the string part starts with a label. */
static auto start_with_label(std::string_view str)
    -> std::variant <std::nullopt_t, std::string_view, ErrorCase> {
    auto pos1 = str.find_first_of('\"');
    auto pos2 = str.find_first_of(':');
    if (pos2 != str.npos && pos2 < pos1) {
        if (have_no_token(str.substr(pos2 + 1)) == false)
            return ErrorCase {};
        return str.substr(0, pos2);
    }
    return std::nullopt;
}

/* Find the first token in the string. */
static auto find_first_token(std::string_view str) -> std::string_view {
    str = remove_front_whitespace(str);
    auto pos = std::ranges::find_if_not(str, [](char c) {
        return std::isspace(c) || c == '#';
    });
    return str.substr(pos - str.begin());
}

/* Check whether the string is a valid token. */
static bool is_valid_token(std::string_view str) {
    for (char c : str)
        if (std::isalnum(c) == false &&
            c != '_' && c != '.' && c != '@')
            return false;
    return true;
}

static bool match_string(std::string_view str, std::initializer_list <std::string_view> list) {
    for (std::string_view s : list) if (str == s) return true;
    return false;
}

Assembly::Assembly(std::string_view file_name) {
    using enum __console::Color;
    this->file_info = std::format("{}file: {}{}",
        __console::color_code <YELLOW>, file_name, __console::color_code <RESET>);

    std::ifstream file { std::string(file_name) };
    panic_if(!file.is_open(), "Failed to open {}", this->file_info);

    this->line_number = 0;

    std::string last;   // Last line
    std::string line;   // Current line

    while (std::getline(file, line)) {
        ++this->line_number;
        try {
            this->parse_line(line);
        } catch (FailToParse &e) {
            if (!e.inner.empty() && e.inner.back() != '\n')
                e.inner += "\n";

            constexpr auto make_line = [] (std::string_view line, std::size_t line_number) {
                return std::format("{: >4}  |  {}", line_number, line);
            };

            std::string line_fmt = make_line(line, this->line_number);
            if (this->line_number != 1)
                line_fmt = std::format("{}\n{}",
                    make_line(last, this->line_number - 1), line_fmt);
            if (std::string temp; std::getline(file, temp))
                line_fmt = std::format("{}\n{}",
                    line_fmt, make_line(temp, this->line_number + 1));

            panic("{:}Failed to parse {}:{}\n{}",
                e.inner, this->file_info, this->line_number, line_fmt);
        } catch(std::exception &e) {
            std::cerr << std::format("Unexpected error: {}\n", e.what());
            runtime_assert(false);
        } catch(...) {
            std::cerr << "Unexpected error\n";
            runtime_assert(false);
        }
        last = std::move(line);
    }
}

/**
 * @brief Parse a line of the assembly.
 * @param line Line of the assembly.
*/
void Assembly::parse_line(std::string_view line) {
    line = remove_front_whitespace(line);

    // Whether the line starts with a label
    if (auto label = start_with_label(line);
        std::holds_alternative<ErrorCase>(label))
        throw FailToParse {};
    else if (std::holds_alternative<std::string_view>(label))
        return this->add_label(std::get<std::string_view>(label));

    auto token = find_first_token(line);

    // Empty line case, nothing happens
    if (token.empty()) return;
    if (!is_valid_token(token)) throw FailToParse {};

    auto rest = line.substr(token.size());
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
void Assembly::add_label(std::string_view label) {
    throw_if <FailToParse> (!is_valid_token(label),
        "Invalid label name: {}", label);

    auto [iter, success] = this->labels.try_emplace(std::string(label));

    throw_if <FailToParse> (success == false && iter->second.line_number != 0,
        "Label \"{}\" already exists\n"
        "First appearance at line: {} in {}",
        label, iter->second.line_number, this->file_info);

    throw_if <FailToParse> (this->current_section == Assembly::Section::INVALID,
        "Label must be defined in a section");

    auto &label_info = iter->second;
    label_info = Assembly::LabelData {
        .line_number = this->line_number,
        .data_index = this->storages.size(),
        .label_name = iter->first,
        .global     = label_info.global,
        .section    = this->current_section
    };
}

/**
 * @brief Parse the first token in the line.
 * @param token Initial token.
 * @param rest Rest of the line.
 * @return Whether the parsing is successful.
 */
void Assembly::parse_storage(std::string_view token, std::string_view rest) {
    if (match_string(token, { "section" })) {
        panic("todo: parse section with name");
    }

    if (match_string(token, { "data", "sdata" })) {
        this->current_section = Assembly::Section::DATA;
    } else if (match_string(token, { "text", "" })) {
        this->current_section = Assembly::Section::TEXT;
    } else if (match_string(token, { "global" })) {
        this->labels[std::string(rest)].global = true;
    } else if (match_string(token, { "align", "p2align" })) {
        // this->storages.push_back(std::make_unique <Alignment>());
    } else if (match_string(token, { "byte" })) {

    } else if (match_string(token, { "short", "half", "2byte" })) {

    } else if (match_string(token, { "long", "word", "4byte" })) {

    } else if (match_string(token, { "type" })) {

    } else if (match_string(token, { "string", "asciz" })) {

    } else if (match_string(token, { "zero" })) {

    }

    // Fail to parse the token
    if (!is_valid_token(rest)) throw FailToParse {};
}

void Assembly::parse_command(std::string_view token, std::string_view rest) {
    panic("todo: parse command");
}


} // namespace dark
