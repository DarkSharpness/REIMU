#pragma once
#include <assembly/storage.h>

namespace dark {

struct OffsetRegister {
    Immediate imm;
    Register  reg;
};

/**
 * Rules:
 * - identifier = integer/char/label (all are valid tokens)
 * - expr = identifier
 *      | +- expr
 *      | expr +- expr
 *      | %rel ( expr )
 *      | ( expr )
 */
struct Parser {
    struct Stop {};

    struct Token {
        enum class Type {
            Identifier,     // integer/float/label
            Operator,       // +, -,
            Parenthesis,    // (, )
            Comma,          // ,
            Character,      // 'c'
            String,         // "string"
            Relocation,     // %hi, %lo, %pcrel_hi, %pcrel_lo
        } type;
        std::string_view what;
    };

    explicit Parser(std::string_view);

    template <typename ..._Args>
    auto match() -> std::tuple <_Args...>;

    auto extract_immediate()        -> Immediate;
    auto extract_register()         -> Register;
    auto extract_offset_register()  -> OffsetRegister;
    auto check_empty()              -> void;
    auto extract_one() noexcept     -> std::optional <Token>;

  private:
    static auto parse_immediate(std::span <Token>)  -> Immediate;

    auto extract_until_splitter()   -> std::span <Token>;
    void skip_comma();

    template <std::size_t _Nm = 0, typename ..._Args>
    auto match_aux(std::tuple <_Args...> &) -> void;

    std::span   <Token> span;
    std::vector <Token> storage;
};

template <std::size_t _Nm, typename ..._Args>
auto Parser::match_aux(std::tuple <_Args...> &ref) -> void {
    constexpr auto _Size = sizeof...(_Args);
    if constexpr (_Nm < _Size) {
        using _Tp = std::tuple_element_t <_Nm, std::tuple <_Args...>>;
        if constexpr (std::same_as <_Tp, Immediate>) {
            std::get <_Nm>(ref) = Parser::extract_immediate();
        } else if constexpr (std::same_as <_Tp, Register>) {
            std::get <_Nm>(ref) = Parser::extract_register();
        } else if constexpr (std::same_as <_Tp, OffsetRegister>) {
            std::get <_Nm>(ref) = Parser::extract_offset_register();
        } else if constexpr (std::same_as <_Tp, Parser::Stop>) {
            static_assert(_Nm + 1 == _Size, "Stop must be the last argument");
            return; // Stop parsing, we are done.
        } else {
            static_assert(sizeof(_Tp) == 0, "Invalid type in Parser::match_aux");
        }

        if constexpr (_Nm + 1 != _Size) {
            this->skip_comma();
            return this->match_aux <_Nm + 1> (ref);
        } else {
            return this->check_empty();
        }
    } else {
        static_assert(_Size == 0, "Invalid number of arguments in Parser::match_aux");
        return this->check_empty();
    }
}

template <typename ..._Args>
auto Parser::match() -> std::tuple <_Args...> {
    std::tuple <_Args...> result;
    this->match_aux(result);
    return result;
}

} // namespace dark
