#pragma once
#include "declarations.h"
#include "interpreter/forward.h"
#include "libc/forward.h"
#include "utility/magic.h"
#include <array>
#include <string_view>

namespace dark::libc {

namespace __details {

template <std::size_t _Nm, auto _Val, auto... Rest>
consteval void nameofs_aux(std::array<std::string_view, _Nm> &names) {
    std::size_t index = _Nm - 1 - sizeof...(Rest);
    names[index]      = meta::nameof<_Val>() | meta::remove_scope;
    if constexpr (sizeof...(Rest) != 0)
        return nameofs_aux<_Nm, Rest...>(names);
}

template <auto... _Vals>
consteval auto nameofs() {
    std::array<std::string_view, sizeof...(_Vals)> names;
    if constexpr (sizeof...(_Vals) != 0)
        nameofs_aux<sizeof...(_Vals), _Vals...>(names);
    return names;
}

#define register_functions(...)                                                                    \
    Function_t __VA_ARGS__;                                                                        \
    enum class _Index : libc_index_t { __VA_ARGS__ };                                              \
    inline constexpr Function_t *funcs[] = {__VA_ARGS__};                                          \
    inline constexpr auto names          = nameofs<__VA_ARGS__>()

register_functions(
    puts, putchar, printf, sprintf, getchar, scanf, sscanf, // IO functions
    malloc, calloc, realloc, free,                          // Memory management
    memset, memcmp, memcpy, memmove,                        // Memory manipulation
    strcpy, strlen, strcat, strcmp                          // Strings manipulation
);

#undef register_functions

} // namespace __details

using __details::funcs;
using __details::names;

static constexpr auto kLibcEnd = kTextStart + std::size(names) * sizeof(command_size_t);

void libc_init(RegisterFile &, Memory &, Device &);

} // namespace dark::libc
