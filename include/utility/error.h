#pragma once
#include "fmtlib.h"
#include <array>
#include <cstdlib>
#include <source_location>
#include <string>
#include <string_view>

namespace std {
extern std::ostream cerr;
} // namespace std

namespace dark {

namespace console {

void flush_stdout();

extern std::ostream error;   // Fatal errors.
extern std::ostream warning; // Warnings.
extern std::ostream message; // Necessary messages.
extern std::ostream profile; // Program runtime information.

enum class Color {
    RED     = 31,
    GREEN   = 32,
    YELLOW  = 33,
    BLUE    = 34,
    MAGENTA = 35,
    CYAN    = 36,
    WHITE   = 37,
    RESET   = 0,
};

template <Color _Color>
static constexpr auto color_code_impl = []() {
    std::array<char, 6> buffer{};
    if constexpr (_Color == Color::RESET) {
        buffer[0] = '\033';
        buffer[1] = '[';
        buffer[2] = '0';
        buffer[3] = 'm';
        buffer[4] = '\0';
    } else {
        auto value = static_cast<int>(_Color);
        buffer[0]  = '\033';
        buffer[1]  = '[';
        buffer[2]  = '0' + (value / 10);
        buffer[3]  = '0' + (value % 10);
        buffer[4]  = 'm';
        buffer[5]  = '\0';
    }
    return buffer;
}();

template <Color _Color>
static constexpr std::string_view color_code = {
    color_code_impl<_Color>.data(), color_code_impl<_Color>.size()
};

} // namespace console

struct PanicError {};

template <typename... Args>
inline static void warning(fmt::format_string<Args...> fmt = "", Args &&...args) {
    console::warning << fmt::format(
        "{}Warning{}: {}\n", console::color_code<console::Color::YELLOW>,
        console::color_code<console::Color::RESET>, fmt::format(fmt, std::forward<Args>(args)...)
    );
}

template <typename... _Args>
[[noreturn]]
inline static void panic(fmt::format_string<_Args...> fmt = "", _Args &&...args) {
    console::flush_stdout();
    console::error << fmt::format(
        "\n{:=^80}\n\n"
        "{}Fatal error{}: {}\n"
        "\n{:=^80}\n",
        "", console::color_code<console::Color::RED>, console::color_code<console::Color::RESET>,
        fmt::format(fmt, std::forward<_Args>(args)...), ""
    );
    throw PanicError{};
}

template <typename _Tp, typename... _Args>
inline static void
panic_if(_Tp &&condition, fmt::format_string<_Args...> fmt = "", _Args &&...args) {
    if (condition) {
        [[unlikely]] return panic(fmt, std::forward<_Args>(args)...);
    }
}

[[noreturn]]
inline static void unreachable(
    std::string message = "", std::source_location where = std::source_location::current()
) {
    // Failure case, print the message and exit
    console::flush_stdout();
    std::cerr << fmt::format("{}"
                             "{}"
                             "Assertion failed at {}:{} in {}:\n"
                             "Internal error, please report this issue to the developer.\n"
                             "{}",
        message, console::color_code<console::Color::RED>, where.file_name(), where.line(),
        where.function_name(), console::color_code<console::Color::RESET>);
    std::exit(EXIT_FAILURE);
}

template <typename _Tp>
inline static void
runtime_assert(_Tp &&condition, std::source_location where = std::source_location::current()) {
    if (!condition) {
        [[unlikely]] return unreachable("", where);
    }
}

} // namespace dark
