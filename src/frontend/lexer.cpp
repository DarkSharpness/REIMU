#include "assembly/frontend/lexer.h"
#include "assembly/exception.h"
#include "assembly/forward.h"
#include <algorithm>

/* Tokenlize the input string.  */
namespace dark::frontend {

static auto extract_str(std::string_view &line, std::size_t n) -> std::string_view {
    auto result = line.substr(0, n);
    line.remove_prefix(n);
    return result;
}

static auto get_first_token(std::string_view &line) -> std::optional<Token> {
    using enum Token::Type;

    constexpr auto __handle_comment  = [](std::string_view &) { return std::nullopt; };
    constexpr auto __handle_operator = [](std::string_view &view) {
        return Token{.type = Operator, .what = extract_str(view, 1)};
    };
    constexpr auto __handle_parentesis = [](std::string_view &view) {
        return Token{.type = Parenthesis, .what = extract_str(view, 1)};
    };
    constexpr auto __handle_comma = [](std::string_view &view) {
        return Token{.type = Comma, .what = extract_str(view, 1)};
    };
    constexpr auto __handle_colon = [](std::string_view &view) {
        return Token{.type = Colon, .what = extract_str(view, 1)};
    };
    constexpr auto __handle_relocation = [](std::string_view &view) {
        auto length = view.find_first_of('(', 1);
        throw_if(length == view.npos, "Expected '(' after relocation operator");
        return Token{.type = Relocation, .what = extract_str(view, length)};
    };
    constexpr auto __handle_character = [](std::string_view &view) {
        throw_if(view.size() < 2, "Expected closing ' for character literal");
        if (view[1] == '\\') {
            throw_if(view.size() < 4, "Expected closing ' for character literal");
            throw_if(view[3] != '\'', "Expected closing ' for character literal");
            return Token{.type = Character, .what = extract_str(view, 4)};
        } else {
            throw_if(view.size() < 3, "Expected closing ' for character literal");
            throw_if(view[2] != '\'', "Expected closing ' for character literal");
            return Token{.type = Character, .what = extract_str(view, 3)};
        }
    };
    constexpr auto __handle_string = [](std::string_view &view) {
        // Find '\' and '"' to handle escape sequences properly
        for (std::size_t i = 1; i < view.size(); i++) {
            if (view[i] == '\\') {
                i++;
            } else if (view[i] == '"') {
                return Token{.type = String, .what = extract_str(view, i + 1)};
            }
        }
        throw FailToParse("Expected closing \" for string literal");
    };
    constexpr auto __handle_identifier = [](std::string_view &view) {
        auto pos    = std::ranges::find_if_not(view, is_label_char);
        auto length = pos - view.begin();
        throw_if(length == 0, "Expected identifier, got nothing");
        return Token{.type = Identifier, .what = extract_str(view, length)};
    };

    while (line.size() && std::isspace(line[0]))
        line.remove_prefix(1);

    if (line.empty())
        return std::nullopt;
    switch (line[0]) {
        case '#':  return __handle_comment(line);
        case '+':
        case '-':  return __handle_operator(line);
        case '(':
        case ')':  return __handle_parentesis(line);
        case '%':  return __handle_relocation(line);
        case '\'': return __handle_character(line);
        case '"':  return __handle_string(line);
        case ',':  return __handle_comma(line);
        case ':':  return __handle_colon(line);
        default:   return __handle_identifier(line);
    }
}

Lexer::Lexer(std::string_view line) {
    std::vector<Token> tokens;

    do {
        auto token = get_first_token(line);
        if (!token.has_value())
            break;
        tokens.push_back(token.value());
    } while (true);

    this->tokens = std::move(tokens);
}

} // namespace dark::frontend
