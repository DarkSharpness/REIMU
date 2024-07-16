#include <assembly/parser.h>
#include <assembly/exception.h>

namespace dark {

static auto extract_str(std::string_view &line, std::size_t n) -> std::string_view {
    auto result = line.substr(0, n);
    line.remove_prefix(n);
    return result;
}

static auto get_first_token(std::string_view &line) -> std::optional <Parser::Token> {
    using Token = Parser::Token;
    using enum Token::Type;

    constexpr auto __handle_comment = [](std::string_view &) {
        return std::nullopt;
    };
    constexpr auto __handle_operator = [](std::string_view &view) {
        return Token { .type = Operator, .what = extract_str(view, 1) };
    };
    constexpr auto __handle_parentesis = [](std::string_view &view) {
        return Token { .type = Parenthesis, .what = extract_str(view, 1) };
    };
    constexpr auto __handle_comma = [](std::string_view &view) {
        return Token { .type = Comma, .what = extract_str(view, 1) };
    };
    constexpr auto __handle_relocation = [](std::string_view &view) {
        auto length = view.find_first_of('(', 1);
        throw_if(length == view.npos, "Expected '(' after relocation operator");
        return Token { .type = Relocation, .what = extract_str(view, length) };
    };
    constexpr auto __handle_character = [](std::string_view &view) {
        throw_if(view.size() < 3, "Expected closing ' for character literal");
        if (view[1] == '\\') {
            throw_if(view.size() < 4, "Expected closing ' for character literal");
            throw_if(view[3] != '\'', "Expected closing ' for character literal");
            return Token { .type = Character, .what = extract_str(view, 4) };
        } else {
            throw_if(view[2] != '\'', "Expected closing ' for character literal");
            return Token { .type = Character, .what = extract_str(view, 3) };
        }
    };
    constexpr auto __handle_string = [](std::string_view &view) {
        // Find '\' and '"' to handle escape sequences properly
        for (std::size_t i = 1 ; i < view.size() ; i++) {
            if (view[i] == '\\') {
                i++;
            } else if (view[i] == '"') {
                return Token { .type = String, .what = extract_str(view, i + 1) };
            }
        }
        throw FailToParse("Expected closing \" for string literal");
    };
    constexpr auto __handle_identifier = [](std::string_view &view) {
        auto pos = std::ranges::find_if_not(view, is_label_char);
        auto length = pos - view.begin();
        return Parser::Token { .type = Identifier, .what = extract_str(view, length) };
    };

    while (line.size() && std::isspace(line[0])) line.remove_prefix(1);

    if (line.empty()) return std::nullopt;
    switch (line[0]) {
        case '#':
            return __handle_comment(line);
        case '+': case '-':
            return __handle_operator(line);
        case '(': case ')':
            return __handle_parentesis(line);
        case '%':
            return __handle_relocation(line);
        case '\'':
            return __handle_character(line);
        case '"':
            return __handle_string(line);
        case ',':
            return __handle_comma(line);
        default:
            return __handle_identifier(line);
    }
}

Parser::Parser(std::string_view line) {
    std::vector <Token> tokens;

    do {
        auto token = get_first_token(line);
        if (!token.has_value()) break;
        tokens.push_back(token.value());
    } while (true);

    this->storage = std::move(tokens);
    this->span   = this->storage;
}

auto Parser::extract_one() noexcept -> std::optional <Token> {
    if (this->span.empty()) return std::nullopt;
    auto token = this->span.front();
    this->span = this->span.subspan(1);
    return token;
}

/**
 * Extract until a comma is found, or the end of the line
 * The comma is not included in the result.
 */
auto Parser::extract_until_splitter() -> std::span <Token> {
    auto pos = std::ranges::find(this->span, Token::Type::Comma, &Token::type);
    auto len = pos - this->span.begin();
    auto ret = this->span.subspan(0, len);
    this->span = this->span.subspan(len);
    return ret;
}

/**
 * Skip the comma, and throw if the next token is not a comma.
 */
auto Parser::skip_comma() -> void {
    auto token = this->extract_one();
    throw_if(!token.has_value(), "Expected comma");
    throw_if(token->type != Token::Type::Comma, "Expected comma");
}

/**
 * Assure that the parse is empty
 */
void Parser::check_empty() {
    throw_if(!this->span.empty(), "Expected end of line");
}

auto Parser::extract_register() -> Register {
    auto tokens = this->extract_until_splitter();
    throw_if(tokens.size() != 1 || tokens[0].type != Token::Type::Identifier,
             "Expected register");
    return sv_to_reg(tokens[0].what);
}

auto Parser::extract_immediate() -> Immediate {
    return Parser::parse_immediate(this->extract_until_splitter());
}

auto Parser::extract_offset_register() -> OffsetRegister {
    auto tokens = this->extract_until_splitter();
    throw_if(tokens.size() < 4, "Expected offset + register");

    /* Pattern: imm(reg), so the last 3 token should be '(' 'reg' ')' */
    auto base = tokens.last(3);
    throw_if(base[0].what != "(" || base[2].what != ")");

    tokens = tokens.first(tokens.size() - 3);

    /**
     * We split the initialization into 2 lines.
     * This is because if exception is thrown when the
     * OffsetRegiste is constructed, its destructor
     * will *not* be called.
     * 
     * If parse_immediate does not throw, but sv_to_reg does,
     * then it may lead to potential memory leak.
     */
    auto reg = sv_to_reg(base[1].what);
    return { Parser::parse_immediate(tokens), reg };
}

} // namespace dark
