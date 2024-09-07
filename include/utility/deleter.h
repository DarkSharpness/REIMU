#pragma once
#include <concepts>
#include <memory>
#include <utility>

namespace dark {

namespace __details {

template <typename _Bp>
struct derival_deleter {
private:
    template <typename _Tp>
        requires std::derived_from<_Tp, _Bp>
    static void _M_deleter(_Bp *ptr) {
        delete static_cast<_Tp *>(ptr);
    }
    using _Fn_t = decltype(&_M_deleter<_Bp>);

public:
    constexpr derival_deleter() noexcept                                   = default;
    constexpr derival_deleter(const derival_deleter &) noexcept            = default;
    constexpr derival_deleter &operator=(const derival_deleter &) noexcept = default;
    template <typename _Tp>
    constexpr derival_deleter(_Tp *) noexcept : _M_deleter_fn(&_M_deleter<_Tp>) {}

    template <typename _Tp>
    constexpr void set_deleter() noexcept {
        this->_M_deleter_fn = &_M_deleter<_Tp>;
    }
    constexpr _Fn_t get_deleter() const noexcept { return this->_M_deleter_fn; }
    constexpr auto operator()(_Bp *ptr) const { return this->_M_deleter_fn(ptr); }
    constexpr void swap(derival_deleter &other) noexcept {
        std::swap(this->_M_deleter_fn, other._M_deleter_fn);
    }
    constexpr bool operator==(const derival_deleter &) const noexcept = default;

private:
    _Fn_t _M_deleter_fn = &_M_deleter<_Bp>;
};

} // namespace __details

template <typename _Bp>
struct derival_ptr : private std::unique_ptr<_Bp, __details::derival_deleter<_Bp>> {
private:
    using Base_t = std::unique_ptr<_Bp, __details::derival_deleter<_Bp>>;

public:
    using Base_t::Base_t;

    template <typename _Tp>
        requires std::derived_from<_Tp, _Bp>
    explicit derival_ptr(_Tp *ptr) noexcept : Base_t(ptr, ptr) {}

    template <typename _Tp>
        requires std::derived_from<_Tp, _Bp>
    derival_ptr(std::unique_ptr<_Tp> ptr) noexcept : derival_ptr(ptr.release()) {}

    using Base_t::operator=;
    using Base_t::get;
    using Base_t::swap;
    using Base_t::operator bool;
    using Base_t::operator*;
    using Base_t::operator->;
    using Base_t::get_deleter;
};

template <typename _Bp>
inline bool operator==(const derival_ptr<_Bp> &lhs, std::nullptr_t) noexcept {
    return lhs.get() == nullptr;
}

template <typename _Bp>
inline bool operator==(std::nullptr_t, const derival_ptr<_Bp> &rhs) noexcept {
    return rhs.get() == nullptr;
}

template <typename _Bp>
inline bool operator==(const derival_ptr<_Bp> &lhs, const derival_ptr<_Bp> &rhs) noexcept {
    return lhs.get() == rhs.get();
}

} // namespace dark
