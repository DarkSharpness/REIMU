#pragma once

namespace dark {

template <typename... _Base>
struct tagged : _Base... {};

namespace __details {

template <typename _Tp>
static constexpr bool with_tag_v = false;
template <typename... _Base>
static constexpr bool with_tag_v<tagged<_Base...>> = true;
template <typename... _Base>
static constexpr bool with_tag_v<tagged<_Base...> &> = true;
template <typename... _Base>
static constexpr bool with_tag_v<const tagged<_Base...>> = true;
template <typename... _Base>
static constexpr bool with_tag_v<const tagged<_Base...> &> = true;

template <typename _Tp>
concept with_tag = with_tag_v<_Tp>;

} // namespace __details

template <typename _F, typename... _Base>
static auto visit(_F &&f, tagged<_Base...> &x) -> void {
    (f(static_cast<_Base &>(x)), ...);
}

template <typename _F, typename... _Base>
static auto visit(_F &&f, const tagged<_Base...> &x) -> void {
    (f(static_cast<const _Base &>(x)), ...);
}

template <typename _F, typename... _Base>
static auto visit(_F &&f, const tagged<_Base...> &x, const tagged<_Base...> &y) -> void {
    (f(static_cast<const _Base &>(x), static_cast<const _Base &>(y)), ...);
}

} // namespace dark
