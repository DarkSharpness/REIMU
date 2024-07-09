#pragma once
#include <interpreter/forward.h>
#include <riscv/register.h>
#include <new>

namespace dark {

struct Executable {
  private:
    static void fn(Executable &, RegisterFile &, Memory &, Device &);

  public:
    using _Func_t = decltype(fn);

    struct MetaData {
        struct PackData;
        Register rd;
        Register rs1;
        Register rs2;
        std::uint32_t imm;
        auto parse(RegisterFile &) const -> PackData;
    };

  private:

    static_assert(sizeof(_Func_t *) == sizeof(std::size_t));

    _Func_t *   func = fn;  // Function pointer.
    MetaData    meta = {};  // Some in hand data.

  public:
    constexpr explicit Executable() = default;

    constexpr void set_handle(_Func_t *func, MetaData meta) {
        this->func = func; this->meta = meta;
    }

    void operator()(RegisterFile &rf, Memory &mem, Device &dev) {
        this->func(*this, rf, mem, dev);
    }

    auto &get_meta() const { return this->meta; }
};

} // namespace dark
