#include <declarations.h>
#include <string_view>

namespace dark {

namespace libc {

constexpr std::string_view names[] = {
    // IO functions
    "puts",
    "putchar",
    "printf",
    "sprintf",
    "getchar",
    "scanf",
    "sscanf",

    // Memory management
    "malloc",
    "calloc",
    "realloc",
    "free",

    // Memory manipulation
    "memcpy",
    "memset",
    "memmove",
    "memcmp",

    // Strings manipulation
    "strcpy",
    "strlen",
    "strnlen_s",
    "strcat",
    "strcmp",
};

constexpr auto kLibcStart = 0x10000;
constexpr auto kLibcEnd = kLibcStart + std::size(libc::names) * sizeof(target_size_t);

} // namespace libc


} // namespace dark
