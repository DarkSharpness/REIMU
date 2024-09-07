#pragma once
#include "declarations.h"
#include <charconv>
#include <concepts>
#include <optional>
#include <string_view>

namespace dark {

template <std::integral _Tp>
inline static auto sv_to_integer(std::string_view view, int base = 10) -> std::optional<_Tp> {
    _Tp result;
    auto res = std::from_chars(view.data(), view.data() + view.size(), result, base);
    if (res.ec == std::errc() && res.ptr == view.end())
        return result;
    else
        return std::nullopt;
}

template <int width, std::unsigned_integral _Int>
static constexpr auto sign_extend(_Int val) -> _Int {
    using _Signed = std::make_signed_t<_Int>;
    struct Pack {
        _Signed val : width;
    };
    return _Int(Pack{.val = _Signed(val)}.val);
}

static constexpr auto split_lo_hi(target_size_t num) {
    struct Result {
        target_size_t lo;
        target_size_t hi;
    };
    return Result{
        .lo = sign_extend<12>(num & 0xFFF),
        .hi = (num + 0x800) >> 12,
    };
}

} // namespace dark
