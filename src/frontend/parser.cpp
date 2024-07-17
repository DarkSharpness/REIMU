#include <assembly/frontend/parser.h>
#include <assembly/frontend/match.h>
#include <assembly/exception.h>
#include <utility.h>

/* The frame of the parser.  */
namespace dark::frontend {

/* Parse exactly one register */
auto parse_register(TokenStream tokens) -> Register {
    throw_if(!match <Token::Type::Identifier> (tokens), "Expected register");
    return sv_to_reg(tokens[0].what);
}

/* Parse exactly one immediate */
auto parse_immediate(TokenStream tokens) -> Immediate {
    ImmediateParser parser(tokens);
    auto imm = parser.parse(tokens);
    return Immediate { std::move(imm) };
}

/* Parse exactly one offset + register */
auto parse_offset_register(TokenStream tokens) -> OffsetRegister {
    throw_if(tokens.size() < 4, "Expected offset + register");

    const auto offset = tokens.split_at(tokens.size() - 3);

    using enum Token::Type;
    throw_if(!match <Parenthesis, Identifier, Parenthesis> (tokens));
    throw_if(tokens[0].what != "(" || tokens[2].what != ")");
    auto reg = sv_to_reg(tokens[1].what);

    return { parse_immediate(offset), reg };
}

} // namespace dark
