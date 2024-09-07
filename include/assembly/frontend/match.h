#pragma once
#include "assembly/frontend/token.h"
#include "assembly/frontend/parser.h"
#include "assembly/exception.h"
#include <algorithm>

namespace dark::frontend {

template <std::size_t _Nm, std::size_t _Size, auto ..._Vals>
inline static auto match_value_aux(TokenStream stream) -> bool {
    if constexpr (_Nm < _Size) {
        constexpr auto _Current = std::get <_Nm>(std::tuple {_Vals...}); 
        if (stream[_Nm].type == _Current)
            return match_value_aux <_Nm + 1, _Size, _Vals...>(stream);
        else
            return false;
    } else {
        return true;
    }
}

template <std::size_t _Nm, std::size_t _Size, typename ..._Args>
inline static void match_type_aux(TokenStream &stream, std::tuple <_Args...> &ref) {
    constexpr auto __consume = [] (TokenStream &stream) {
        throw_if(stream.empty() || stream[0].type != Token::Type::Comma,
            "Expected a comma after the previous argument");
        stream = stream.subspan(1);
    };

    if constexpr (_Nm < _Size) {
        if constexpr (_Nm > 0) __consume(stream);

        // Extract the content until the next comma.
        auto pos = std::ranges::find(stream, Token::Type::Comma, &Token::type);
        const auto content = stream.split_at(pos - stream.begin());

        using _Tp = std::tuple_element_t <_Nm, std::tuple <_Args...>>;
        if constexpr (std::same_as <_Tp, Register>) {
            std::get <_Nm>(ref) = parse_register(content);
        } else if constexpr (std::same_as <_Tp, Immediate>) {
            std::get <_Nm>(ref) = parse_immediate(content);
        } else if constexpr (std::same_as <_Tp, OffsetRegister>) {
            std::get <_Nm>(ref) = parse_offset_register(content);
        } else {
            static_assert(sizeof(_Tp) == 0, "Invalid type in match_type_aux");
        }

        match_type_aux <_Nm + 1, _Size> (stream, ref);
    } else {
        if constexpr (_Size == sizeof...(_Args))
            throw_if(!stream.empty(), "Expected end of line");
        else// Note that placeholder will also consume a comma.
            __consume(stream);
    }
}

template <typename ..._Args>
[[nodiscard]]
inline static auto match(TokenStream &stream) -> std::tuple <_Args...> {
    std::tuple <_Args...> result;
    constexpr auto _Size = sizeof...(_Args);
    if constexpr (_Size == 0) {
        throw_if(!stream.empty(), "Expected end of line");
    } else {
        using _LastTp = std::tuple_element_t <_Size - 1, std::tuple <_Args...>>;
        if constexpr (!std::same_as <_LastTp, Token::Placeholder>) {
            match_type_aux <0, _Size, _Args...> (stream, result);
        } else {
            match_type_aux <0, _Size - 1, _Args...> (stream, result);
        }
    }
    return result;
}

template <auto ..._Vals>
requires (std::same_as <Token::Type, decltype(_Vals)> && ...) && (sizeof...(_Vals) > 0)
[[nodiscard]]
inline static auto match(const TokenStream stream) noexcept -> bool {
    constexpr auto _Size = sizeof...(_Vals);
    if constexpr (_Size == 0) {
        return stream.empty();
    } else {
        constexpr auto _LastVal = std::get <_Size - 1>(std::tuple {_Vals...});
        if constexpr (_LastVal != Token::Type::Placeholder) {
            if (stream.size() != _Size) return false;
            return match_value_aux <0, _Size, _Vals...> (stream);
        } else {
            if (stream.size() < _Size - 1) return false;
            return match_value_aux <0, _Size - 1, _Vals...> (stream);
        }
    }
}

} // namespace dark::frontend
