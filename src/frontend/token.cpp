#include "assembly/frontend/token.h"
#include <algorithm>

namespace dark::frontend {

auto TokenStream::count_args() const -> std::size_t {
    if (this->empty())
        return 0;
    return std::ranges::count(*this, Token::Type::Comma, &Token::type) + 1;
}

} // namespace dark::frontend
