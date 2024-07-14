#pragma once
#include <declarations.h>
#include <vector>
#include <iostream>
#include <source_location>
#include <fmtlib>
#include <optional>
#include <charconv>
#include <ranges>

namespace dark {

namespace __console {

enum class Color {
    RED         = 31,
    GREEN       = 32,
    YELLOW      = 33,
    BLUE        = 34,
    MAGENTA     = 35,
    CYAN        = 36,
    WHITE       = 37,
    RESET       = 0,
};

template <Color _Color>
static constexpr auto color_code_impl = []() {
    std::array <char, 6> buffer {};
    if constexpr (_Color == Color::RESET) {
        buffer[0] = '\033';
        buffer[1] = '[';
        buffer[2] = '0';
        buffer[3] = 'm';
        buffer[4] = '\0';
    } else {
        auto value = static_cast<int>(_Color);
        buffer[0] = '\033';
        buffer[1] = '[';
        buffer[2] = '0' + (value / 10);
        buffer[3] = '0' + (value % 10);
        buffer[4] = 'm';
        buffer[5] = '\0';
    }
    return buffer;
} ();

template <Color _Color>
static constexpr std::string_view color_code = {
    color_code_impl <_Color>.data(), color_code_impl <_Color>.size()
};

} // namespace __console

// Global visibility, one definition.
inline bool warning_shutdown {};

template <typename ...Args>
inline static void warning(std::format_string <Args...> fmt = "", Args &&...args) {
    if (warning_shutdown) return;
    std::cerr
        << __console::color_code <__console::Color::YELLOW>
        << "Warning: "
        << __console::color_code <__console::Color::RESET>
        << std::format(fmt, std::forward <Args>(args)...)
        << std::endl;
}

template <typename ...Args>
inline static void warning_if(bool condition, std::format_string <Args...> fmt = "", Args &&...args) {
    if (condition) return warning(fmt, std::forward <Args>(args)...);
}

template <typename ..._Args>
__attribute__((noinline, noreturn, cold))
inline static void panic(std::format_string <_Args...> fmt = "", _Args &&...args) {
    // Failure case, print the message and exit
    std::cerr
        << std::format("\n{:=^80}\n\n", "")
        << __console::color_code <__console::Color::RED>
        << "Fatal error: "
        << __console::color_code <__console::Color::RESET>
        << std::format(fmt, std::forward <_Args>(args)...)
        << __console::color_code <__console::Color::RESET>
        << std::format("\n\n{:=^80}\n", "");
    std::exit(EXIT_FAILURE);
}

template <typename _Tp, typename ..._Args>
__attribute((always_inline))
inline static void panic_if(_Tp &&condition, std::format_string <_Args...> fmt = "", _Args &&...args) {
    if (condition) return panic(fmt, std::forward <_Args>(args)...);
}

template <typename _Execption, typename _Tp, typename ..._Args>
__attribute((always_inline))
inline static void throw_if(_Tp &&condition, std::format_string <_Args...> fmt = "", _Args &&...args) {
    if (condition) throw _Execption(std::format(fmt, std::forward <_Args>(args)...));
}

/**
 * @brief Runtime unreachable, exit the program with a message.
 */
__attribute__((noinline, noreturn, cold))
inline static void unreachable(std::source_location where = std::source_location::current()) {
    // Failure case, print the message and exit
    std::cerr << std::format(
        "{}"
        "Assertion failed at {}:{} in {}:\n"
        "Internal error, please report this issue to the developer.\n"
        "{}",
        __console::color_code <__console::Color::RED>,
        where.file_name(), where.line(), where.function_name(),
        __console::color_code <__console::Color::RESET>);
    std::exit(EXIT_FAILURE);
}

/**
 * @brief Runtime assertion, if the condition is false, print the message and exit.
 * @param condition Assertion condition.
 */
template <typename _Tp>
inline static void runtime_assert(_Tp &&condition, std::source_location where = std::source_location::current()) {
    if (condition) return;
    // Failure case, print the message and exit
    return unreachable(where);
}

template <std::integral _Tp>
inline static auto sv_to_integer(std::string_view view, int base = 10) -> std::optional <_Tp> {
    _Tp result;
    auto res = std::from_chars(view.data(), view.data() + view.size(), result, base);
    if (res.ec == std::errc() && res.ptr == view.end())
        return result;
    else
        return std::nullopt;
}

static constexpr auto split_lo_hi(target_size_t num) {
    struct Result {
        target_size_t lo;
        target_size_t hi;
    };
    return Result {
        .lo = num & 0xFFF,
        .hi = (num + 0x800) >> 12
    };
}

namespace __hash {

template <std::size_t _Base = 131, std::size_t _Mod = 0>
static constexpr auto switch_hash_impl(std::string_view view) {
    auto hash = std::size_t {0};
    for (char c : view) {
        hash = hash * _Base + c;
        if constexpr (_Mod != 0) hash %= _Mod;
    }
    return hash;
}


} // namespace __hash

} // namespace dark
