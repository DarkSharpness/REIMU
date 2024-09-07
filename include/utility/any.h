#pragma once
#include <concepts>
#include <typeinfo>
#include <utility>

namespace dark {

struct any;

namespace __details {

template <typename _Tp>
concept any_type = std::same_as<any, std::decay_t<_Tp>>;

template <typename _Tp>
concept non_any_type = !any_type<_Tp>;

struct any_base {
    virtual ~any_base() = default;
};

template <typename _Tp>
struct any_object : any_base {
    using _Dp = std::decay_t<_Tp>;
    static_assert(std::same_as<_Tp, _Dp>);
    static_assert(non_any_type<_Tp>);
    _Tp _M_value;
    template <typename... _Args>
    any_object(_Args &&...args) : _M_value(std::forward<_Args>(args)...) {}
};

} // namespace __details

struct any {
private:
    __details::any_base *_M_ptr;

    template <typename _Tp>
    auto *_M_force_cast() const {
        using _Dp = std::decay_t<_Tp>;
        auto *ptr = dynamic_cast<__details::any_object<_Dp> *>(_M_ptr);
        if (!ptr)
            throw std::bad_cast();
        return ptr;
    }

public:
    any() : _M_ptr(nullptr) {}
    any(const any &other)            = delete;
    any &operator=(const any &other) = delete;

    any(any &&other) noexcept : _M_ptr(other._M_ptr) { other._M_ptr = nullptr; }

    template <__details::non_any_type _Tp>
    any(_Tp &&value) :
        _M_ptr(new __details::any_object<std::decay_t<_Tp>>(std::forward<_Tp>(value))) {}

    any &operator=(any &&other) noexcept {
        if (this != &other) {
            delete _M_ptr;
            _M_ptr       = other._M_ptr;
            other._M_ptr = nullptr;
        }
        return *this;
    }

    template <__details::non_any_type _Tp>
    any &operator=(_Tp &&value) {
        delete _M_ptr;
        _M_ptr = new __details::any_object<std::decay_t<_Tp>>(std::forward<_Tp>(value));
        return *this;
    }

    template <__details::non_any_type _Tp>
    _Tp get() {
        return this->_M_force_cast<_Tp>()->_M_value;
    }

    template <__details::non_any_type _Tp>
    const _Tp get() const {
        return this->_M_force_cast<_Tp>()->_M_value;
    }

    template <__details::non_any_type _Tp, typename... _Args>
    _Tp &emplace(_Args &&...args) {
        delete _M_ptr;
        auto *ptr    = new __details::any_object<_Tp>(std::forward<_Args>(args)...);
        this->_M_ptr = ptr;
        return ptr->_M_value;
    }

    ~any() { delete _M_ptr; }
};

} // namespace dark
