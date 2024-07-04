#pragma once
#include <interpreter/forward.h>
#include <new>

namespace dark {

struct Executable {
  private:
    static void fn(Executable &, RegisterFile &, Memory &, Device &);

  public:
    using _Func_t = decltype(fn);

  private:

    static_assert(sizeof(_Func_t *) == sizeof(std::size_t));

    _Func_t *   func = fn;  // Function pointer.
    std::size_t data = 0;   // Some in hand data.

  public:
    constexpr explicit Executable() = default;

    constexpr void set_handle(_Func_t *func, std::size_t data) {
        this->func = func;
        this->data = data;
    }

    void operator()(RegisterFile &rf, Memory &mem, Device &dev) {
        this->func(*this, rf, mem, dev);
    }

    template <typename _Tp>
    const _Tp &get_data() const {
        return *std::launder(reinterpret_cast<const _Tp *>(&this->data));
    }
};

} // namespace dark
