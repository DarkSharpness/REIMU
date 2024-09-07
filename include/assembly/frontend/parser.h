#pragma once
#include "assembly/frontend/token.h"
#include "assembly/storage/immediate.h"
#include <unordered_map>

namespace dark::frontend {

struct OffsetRegister {
    Immediate imm;
    Register  reg;
};

auto parse_register(TokenStream)  -> Register;
auto parse_immediate(TokenStream) -> Immediate;
auto parse_offset_register(TokenStream) -> OffsetRegister;
auto try_parse_offset_register(TokenStream tokens) -> std::optional<OffsetRegister>;

struct ImmediateParser {
    using Owner_t = std::unique_ptr<ImmediateBase>;

    explicit ImmediateParser(TokenStream);
    auto parse(TokenStream) const -> Owner_t;

private:
    auto find_right_parenthesis(TokenStream &view) const -> TokenStream;
    auto find_single_op(TokenStream &) const -> Owner_t;
    std::unordered_map <const Token *, std::size_t> matched;
};


} // namespace dark::frontend
