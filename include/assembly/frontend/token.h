#pragma once
#include <cstddef>
#include <span>
#include <string_view>

namespace dark::frontend {

struct Token {
    enum class Type {
        Identifier,     // integer/float/label
        Operator,       // +, -,
        Parenthesis,    // (, )
        Comma,          // ,
        Colon,          // :
        Character,      // 'c'
        String,         // "string"
        Relocation,     // %hi, %lo, %pcrel_hi, %pcrel_lo
        Placeholder,    // A flag used in pattern matching
    } type;
    struct Placeholder {};
    std::string_view what;
};

struct TokenStream : std::span <const Token> {
    using std::span <const Token>::span;
    TokenStream(std::span <const Token> view) : span(view) {}

    auto split_at(std::size_t len) -> TokenStream {
        auto ret = this->subspan(0, len);
        static_cast <std::span <const Token> &>(*this) = this->subspan(len);
        return TokenStream {ret};
    }

    auto count_args() const -> std::size_t;
};

} // namespace dark::frontend
