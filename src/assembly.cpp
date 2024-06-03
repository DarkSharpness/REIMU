#include "utility.h"
#include "assembly.h"
#include "default_config.h"
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

    IntegerData(std::size_t n, std::string_view data) : data(data) {
        switch (n) {
            case 1: this->type = Type::BYTE; break;
            case 2: this->type = Type::SHORT; break;
            case 4: this->type = Type::LONG; break;
            default: runtime_assert(false);
        }
    }

    ~IntegerData() override = default;
};

struct ZeroBytes : public Assembly::Storage {
    std::size_t size;
    explicit ZeroBytes(std::size_t size) : size(size) {}
    ~ZeroBytes() override = default;
};

struct ASCIZ : public Assembly::Storage {
    std::string data;
    explicit ASCIZ(std::string str) : data(std::move(str)) {}
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
    -> std::variant <std::monostate, std::string_view, ErrorCase> {
    auto pos1 = str.find_first_of('\"');
    auto pos2 = str.find_first_of(':');
    if (pos2 != str.npos && pos2 < pos1) {
        if (have_no_token(str.substr(pos2 + 1)) == false)
            return ErrorCase {};
        return str.substr(0, pos2);
    }
    return std::monostate {};
}

/* Find the first token in the string. */
static auto find_first_token(std::string_view str)
    -> std::pair <std::string_view, std::string_view> {
    str = remove_front_whitespace(str);
    auto pos = std::ranges::find_if(str, [](char c) {
        return std::isspace(c) || c == '#';
    });
    return { str.substr(0, pos - str.begin()), str.substr(pos - str.begin()) };
}

static auto find_first_asciz(std::string_view str)
    -> std::pair <std::string, std::string_view> {
    str = remove_front_whitespace(str);

    enum class _Error_t {
        INVALID = 0,
        ESCAPE  = 1,
        NOEND   = 2,
    };
    using enum _Error_t;

    constexpr auto __throw_invalid = [][[noreturn]](_Error_t num) {
        switch (num) {
            case _Error_t::INVALID: throw FailToParse { "Invalid ascii string" };
            case _Error_t::ESCAPE:  throw FailToParse { "Invalid escape character" };
            case _Error_t::NOEND:   throw FailToParse { "Missing end of string" };
            default: runtime_assert(false); __builtin_unreachable();
        }
    };

    if (str.empty() || str.front() != '\"') __throw_invalid(INVALID);

    // Parse the string
    std::string ret;
    for (std::size_t i = 1; i < str.size() ; ++i) {
        switch (str[i]) {
            case '\\':
                switch (str[++i]) { // Even safe when out of bound ('\0' case)
                    case 'n': ret.push_back('\n'); break;
                    case 't': ret.push_back('\t'); break;
                    case 'r': ret.push_back('\r'); break;
                    case '0': ret.push_back('\0'); break;
                    case '\\': ret.push_back('\\'); break;
                    case '\"': ret.push_back('\"'); break;
                    default: __throw_invalid(ESCAPE);
                } break;
            case '\"': return { ret, str.substr(i + 1) };
            default: ret.push_back(str[i]);
        }
    }
    __throw_invalid(NOEND);
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

static bool match_prefix(std::string_view str, std::initializer_list <std::string_view> list) {
    for (std::string_view s : list) if (str.starts_with(s)) return true;
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

    // Not a label, find the first token
    auto [token, rest] = find_first_token(line);

    // Empty line case, nothing happens
    if (token.empty()) return;
    if (!is_valid_token(token)) throw FailToParse {};

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
        "First appearance at line {} in {}",
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
    constexpr auto __parse_section = [](std::string_view rest) -> std::string_view {
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
        return std::string_view {};
    };

    if (match_string(token, { "section" })) {
        token = __parse_section(rest);
        rest = std::string_view {};
        if (token.empty()) return;
    }

    auto new_rest = this->parse_storage_impl(token, rest);

    // Fail to parse the token
    if (!have_no_token(new_rest)) throw FailToParse {};
}

auto Assembly::parse_storage_impl(std::string_view token, std::string_view rest) -> std::string_view {
    // Warn once for those known yet ignored attributes.
    constexpr auto __warn_once = [](std::string_view str) {
        static std::unordered_set <std::string> ignored_attributes;
        if (ignored_attributes.insert(std::string(str)).second)
            warning("attribute ignored: .{}", str);
        return std::string_view {};
    };
    constexpr auto __set_section = [](Assembly *ptr, std::string_view rest, Assembly::Section section) {
        ptr->current_section = section;
        return rest;
    };
    constexpr auto __set_global = [](Assembly *ptr, std::string_view rest) {
        ptr->labels[std::string(rest)].global = true;
        return std::string_view {};
    };
    constexpr auto __set_align = [](Assembly *ptr, std::string_view rest) {
        using __config::kMaxAlign;
        auto [num_str, new_rest] = find_first_token(rest);
        auto num = sv_to_integer <std::size_t> (num_str).value_or(kMaxAlign);
        throw_if <FailToParse> (num >= kMaxAlign, "Invalid alignment value: \"{}\"", rest);
        ptr->storages.push_back(std::make_unique <Alignment> (std::size_t{1} << num));
        return new_rest;
    };
    constexpr auto __set_bytes = [](Assembly *ptr, std::string_view rest, std::size_t n) {
        auto [first, new_rest] = find_first_token(rest);
        ptr->storages.push_back(std::make_unique <IntegerData> (n, first));
        return new_rest;
    };
    constexpr auto __set_asciz = [](Assembly *ptr, std::string_view rest) {
        auto [str, new_rest] = find_first_asciz(rest);
        ptr->storages.push_back(std::make_unique <ASCIZ> (std::move(str)));
        return new_rest;
    };
    constexpr auto __set_zeros = [](Assembly *ptr, std::string_view rest) {
        auto [num_str, new_rest] = find_first_token(rest);
        using __config::kMaxZeros;
        auto num = sv_to_integer <std::size_t> (num_str).value_or(kMaxZeros);
        throw_if <FailToParse> (num >= kMaxZeros, "Invalid zero count: \"{}\"", num_str);
        ptr->storages.push_back(std::make_unique <ZeroBytes> (num));
        return new_rest;
    };

    using namespace ::dark::literals;

    #define match_or_break(str, expr) case switch_hash_impl(str):\
         if (token == str) { return expr; } else break

    switch (switch_hash_impl(token)) {
        // Data section
        match_or_break("data", __set_section(this, rest, Assembly::Section::DATA));
        match_or_break("sdata", __set_section(this, rest , Assembly::Section::DATA));
        match_or_break("bss", __set_section(this, rest, Assembly::Section::BSS));
        match_or_break("sbss", __set_section(this, rest, Assembly::Section::BSS));
        match_or_break("rodata", __set_section(this, rest, Assembly::Section::RODATA));
        match_or_break("text", __set_section(this, rest, Assembly::Section::TEXT));

        // Real storage
        match_or_break("align", __set_align(this, rest));
        match_or_break("p2align", __set_align(this, rest));
        match_or_break("byte", __set_bytes(this, rest, 1));
        match_or_break("short", __set_bytes(this, rest, 2));
        match_or_break("half", __set_bytes(this, rest, 2));
        match_or_break("2byte", __set_bytes(this, rest, 2));
        match_or_break("long", __set_bytes(this, rest, 4));
        match_or_break("word", __set_bytes(this, rest, 4));
        match_or_break("4byte", __set_bytes(this, rest, 4));
        match_or_break("string", __set_asciz(this, rest));
        match_or_break("asciz", __set_asciz(this, rest));
        match_or_break("zero", __set_zeros(this, rest));
        match_or_break("globl", __set_global(this, rest));

        // Only warn once for those known yet ignored attributes.
        case "type"_h:
        case "size"_h:
        case "file"_h:
        case "attribute"_h:
        case "addrsig"_h:
            return __warn_once(token);

        default:
            // We allow cfi directives to be ignored. 
            if (!token.starts_with("cfi_")) break;
            return __warn_once(token);
    }

    throw FailToParse { std::format("Unknown storage type: .{}", token) };
}


void Assembly::parse_command(std::string_view token, std::string_view rest) {
    // panic("todo: parse command");
}


} // namespace dark
