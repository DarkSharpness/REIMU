#pragma once
#include <source_location>
#include <string_view>

namespace dark::meta {

namespace __detail::__meta {

template <auto _Val>
consteval auto func_info() {
    return std::string_view{std::source_location::current().function_name()};
}

template <class _Tp>
consteval auto func_info() {
    return std::string_view{std::source_location::current().function_name()};
}

consteval auto cstrlen(const char *ptr) {
    const char *end = ptr;
    while (*end)
        ++end;
    return end - ptr;
}

struct info {
    size_t left_omit, right_omit;
};

consteval auto get_type_blank() {
    const auto result = func_info<void>();
    const auto l      = result.find("void");
    if (l == std::string_view::npos)
        return info{0, 0};
    const auto r = result.size() - l - cstrlen("void");
    return info{l, r};
}

consteval auto get_value_blank() {
    const auto result = func_info<nullptr>();
    const auto l      = result.find("nullptr");
    if (l == std::string_view::npos)
        return info{0, 0};
    const auto r = result.size() - l - cstrlen("nullptr");
    return info{l, r};
}

} // namespace __detail::__meta

template <auto _Val>
constexpr auto value_string() {
    const auto func   = __detail::__meta::func_info<_Val>();
    const auto [l, r] = __detail::__meta::get_value_blank();
    auto name         = func.substr(l);
    name.remove_suffix(r);
    return name;
}

template <class _Tp>
constexpr auto type_string() {
    const auto func   = __detail::__meta::func_info<_Tp>();
    const auto [l, r] = __detail::__meta::get_type_blank();
    auto name         = func.substr(l);
    name.remove_suffix(r);
    return name;
}

/* Name of the type in constexpr context. */
template <auto _Val>
constexpr auto nameof() {
    return value_string<_Val>();
}

/* Name of the type in constexpr context. */
template <class _Tp>
constexpr auto nameof() {
    return type_string<_Tp>();
}

/* A function-programming style class, remove all scopes.  */
static constexpr struct remove_scope_t {
    /* We utilize the fact that npos == -1 */
    static_assert(std::string_view::npos + 1 == 0);
    constexpr auto operator()(std::string_view name) const {
        return name.substr(name.find_last_of(':') + 1);
    }
} remove_scope;

constexpr auto operator|(remove_scope_t, std::string_view name) {
    return remove_scope(name);
}
constexpr auto operator|(std::string_view name, remove_scope_t) {
    return remove_scope(name);
}

} // namespace dark::meta

// /**
//  * A function-like macro for dark::meta::nameof.
//  * It can be used to print the name of:
//  * - any type
//  * - constexpr value
//  */
// #define nameof(x) dark::meta::nameof <x> ()
