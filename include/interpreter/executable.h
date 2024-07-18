#pragma once
#include <interpreter/forward.h>
#include <riscv/register.h>
#include <new>

namespace dark {

struct Executable {
  private:
    [[noreturn]]
    static auto fn(Executable &, RegisterFile &, Memory &, Device &) -> Executable *;

  public:
    using _Func_t = decltype(fn);

    struct MetaData {
        struct PackData;
        Register rd  {};
        Register rs1 {};
        Register rs2 {};
        std::uint32_t imm {};
        auto parse(RegisterFile &) const -> PackData;
    };

  private:
    static_assert(sizeof(_Func_t *) == sizeof(std::size_t));

    _Func_t *   func = fn;  // Function pointer.
    MetaData    meta = {};  // Some in hand data.

  public:
    constexpr explicit Executable() = default;
    constexpr explicit Executable(_Func_t *func, MetaData meta) : func(func), meta(meta) {}

    Executable(const Executable &) = delete;
    Executable &operator = (const Executable &) = delete;

    void set_handle(_Func_t *func, MetaData meta) {
        this->func = func;
        this->meta = meta;
    }

    auto operator()(RegisterFile &rf, Memory &mem, Device &dev) {
        return this->func(*this, rf, mem, dev);
    }

    auto &get_meta() const { return this->meta; }

    auto next(target_size_t n = 4) -> Executable * {
        static_assert(sizeof(command_size_t) == 4,
            "We assume that the size of command is 4 bytes.");
        return n % 4 == 0 ?
            this + (target_ssize_t(n) >> 2) :nullptr; 
    }
};

auto compile_once(Executable &, RegisterFile &, Memory &, Device &) -> Executable *;

} // namespace dark
