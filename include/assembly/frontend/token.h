#pragma once
#include <assembly/forward.h>
#include <span>

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

    template <typename _Tp>
    requires (!std::same_as <TokenStream, _Tp> &&
        std::constructible_from <std::span <const Token>, _Tp>)
    TokenStream(_Tp && view) : std::span <const Token> {std::forward <_Tp> (view)} {}

    auto split_at(std::size_t len) -> TokenStream {
        auto ret = this->subspan(0, len);
        static_cast <std::span <const Token> &>(*this) = this->subspan(len);
        return TokenStream {ret};
    }
};

} // namespace dark::frontend
