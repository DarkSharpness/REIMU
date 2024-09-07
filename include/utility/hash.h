#pragma once
#include <string_view>

namespace dark::hash {

template <std::size_t _Base = 131, std::size_t _Mod = 0>
static constexpr auto switch_hash_impl(std::string_view view) {
    auto hash = std::size_t{0};
    for (char c : view) {
        hash = hash * _Base + c;
        if constexpr (_Mod != 0)
            hash %= _Mod;
    }
    return hash;
}

} // namespace dark::hash
