#pragma once
#include <assembly/forward.h>
#include <assembly/frontend/token.h>
#include <riscv/register.h>
#include <vector>

namespace dark::frontend {

struct Lexer {
    explicit Lexer(std::string_view);
    auto get_stream() const -> TokenStream { return this->tokens; }
private:
    std::vector <Token> tokens;
};

} // namespace dark::frontend
