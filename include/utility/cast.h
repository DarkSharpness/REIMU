#pragma once
#include <declarations.h>
#include <string_view>
#include <optional>
#include <charconv>

namespace dark {

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

} // namespace dark
