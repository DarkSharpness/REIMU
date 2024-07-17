#include <utility.h>
#include <assembly/frontend/match.h>
#include <assembly/assembly.h>
#include <assembly/exception.h>
#include <assembly/storage.h>
#include <fstream>
#include <algorithm>

namespace dark {

using frontend::Token;
using frontend::TokenStream;
using frontend::match;
using enum frontend::Token::Type;

static auto get_section(TokenStream tokens) -> std::string_view {
    throw_if(tokens.empty(), "Missing section name");
    auto section_name = tokens[0].what;
    if (section_name.starts_with(".text"))  return "text";
    if (section_name.starts_with(".data"))  return "data";
    if (section_name.starts_with(".sdata")) return "data";
    if (section_name.starts_with(".bss"))   return "bss";
    if (section_name.starts_with(".sbss"))  return "bss";
    if (section_name.starts_with(".rodata")) return "rodata";
    return "unknown";
}

template <Token::Type _What>
static auto get_single(TokenStream tokens) -> std::string_view {
    throw_if(!match <_What> (tokens), "Invalid token");
    return tokens[0].what;
}

static auto parse_asciz(std::string_view str) -> std::string {
    enum class _Error_t {
        INVALID = 0,
        ESCAPE  = 1,
        NOEND   = 2,
    };
    using enum _Error_t;

    constexpr auto __throw_invalid = [][[noreturn]](_Error_t num) {
        switch (num) {
            case INVALID: throw FailToParse { "Invalid ascii string" };
            case ESCAPE:  throw FailToParse { "Invalid escape character" };
            case NOEND:   throw FailToParse { "Missing end of string" };
            default: unreachable();
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
            case '\"':
                throw_if(i + 1 != str.size(), "Unexpected character after '\"'");
                return ret;
            default: ret.push_back(str[i]);
        }
    }
    __throw_invalid(NOEND);
}

void Assembler::parse_storage_new(std::string_view token, const TokenStream &rest) {
    constexpr auto __set_section = [](Assembler *ptr, TokenStream, Section section) {
        ptr->set_section(section);
    };
    constexpr auto __set_globl = [](Assembler *ptr, TokenStream rest) {
        auto name = get_single <Identifier> (rest);
        ptr->labels[std::string(name)].set_global(ptr->line_number);
    };
    constexpr auto __set_align = [](Assembler *ptr, TokenStream rest) {
        constexpr std::size_t kMaxAlign = 20;
        auto name = get_single <Identifier> (rest);
        auto num = sv_to_integer <std::size_t> (name).value_or(kMaxAlign);
        throw_if(num >= kMaxAlign, "Invalid alignment value.");
        ptr->push_cmd <Alignment> (std::size_t{1} << num);
    };
    constexpr auto __set_bytes = [](Assembler *ptr, TokenStream rest, IntegerData::Type n) {
        auto [imm] = match <Immediate> (rest);
        ptr->push_cmd <IntegerData> (imm, n);
    };
    constexpr auto __set_asciz = [](Assembler *ptr, TokenStream rest) {
        auto name = get_single <String> (rest);
        ptr->push_cmd <ASCIZ> (parse_asciz(name));
    };
    constexpr auto __set_zeros = [](Assembler *ptr, TokenStream rest) {
        auto name = get_single <Identifier> (rest);
        constexpr std::size_t kMaxZeros = std::size_t{1} << 20;
        auto num = sv_to_integer <std::size_t> (name).value_or(kMaxZeros);
        throw_if(num >= kMaxZeros, "Invalid zero count: \"{}\"", name);
        ptr->push_cmd <ZeroBytes> (num);
    };
    // constexpr auto __set_label = [](Assembler *ptr, std::string_view rest) {
    //     auto [label, new_rest] = find_first_token(rest);
    //     throw_if(!new_rest.starts_with(','));
    //     new_rest.remove_prefix(1);
    //     new_rest = remove_front_whitespace(new_rest);
    //     new_rest = remove_back_whitespace(new_rest);
    //     /* Allow new rest = . or . + 0 */
    //     if (new_rest == "." || new_rest == ". + 0") {
    //         ptr->add_label(label);
    //         return std::string_view {};
    //     }
    //     throw FailToParse {};
    // };

    // Warn once  those known yet ignored attributes.
    constexpr auto __warn_once = [](std::string_view str) {
        static std::unordered_set <std::string> ignored_attributes;
        if (ignored_attributes.insert(std::string(str)).second)
            warning("attribute ignored: .{}", str);
    };

    token.remove_prefix(1); // remove the dot
    if (token == "section") token = get_section(rest);

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
        // match_or_break("set",       __set_label);

        default: break;
    }
    #undef match_or_break
    return __warn_once(token);
}

} // namespace dark