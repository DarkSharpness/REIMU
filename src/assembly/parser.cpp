#include <assembly/parser.h>
#include <assembly/exception.h>

/* The frame of the parser.  */
namespace dark {

/* Extract exactly one token. */
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

/* Skip the comma, and throw if the next token is not a comma. */
auto Parser::skip_comma() -> void {
    auto token = this->extract_one();
    throw_if(!token.has_value(), "Expected comma");
    throw_if(token->type != Token::Type::Comma, "Expected comma");
}

/* Assure that the parse is empty */
auto Parser::check_empty() -> void {
    throw_if(!this->span.empty(), "Expected end of line");
}

/* Extract exactly one register */
auto Parser::extract_register() -> Register {
    auto tokens = this->extract_until_splitter();
    throw_if(tokens.size() != 1 || tokens[0].type != Token::Type::Identifier,
             "Expected register");
    return sv_to_reg(tokens[0].what);
}

/* Extract exactly one immediate */
auto Parser::extract_immediate() -> Immediate {
    return Parser::parse_immediate(this->extract_until_splitter());
}

/**
 * Extract exactly one offset + register
 */
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
