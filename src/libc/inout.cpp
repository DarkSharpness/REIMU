#include "declarations.h"
#include "interpreter/device.h"
#include "interpreter/exception.h"
#include "interpreter/memory.h"
#include "interpreter/register.h"
#include "libc/libc.h"
#include "libc/utility.h"
#include <charconv>
#include <cstddef>
#include <limits>
#include <sstream>
#include <utility>

namespace dark::libc::__details {

template <_Index index>
static void checked_printf_impl(
    RegisterFile &rf, Memory &mem, std::ostream &out, std::string_view fmt, Register from
) {
    auto reg             = reg_to_int(from);
    const auto extra_arg = [&]() {
        if (reg == reg_to_int(Register::a7) + 1)
            throw FailToInterpret{
                .error = Error::NotImplemented, .message = "Too many arguments for printf"
            };
        return rf[int_to_reg(reg++)];
    };

    for (std::size_t i = 0; i < fmt.size(); ++i) {
        char c = fmt[i];
        if (c != '%') {
            out.put(c);
            continue;
        }
        // c == '%' here
        switch (fmt[++i]) {
            case 'd': out << std::dec << static_cast<std::int32_t>(extra_arg()); break;
            case 's': out << checked_get_string<_Index::printf>(mem, extra_arg()); break;
            case 'c': out.put(static_cast<char>(extra_arg())); break;
            case 'x': out << std::hex << extra_arg(); break;
            case 'p': out << "0x" << std::hex << extra_arg(); break;
            case 'u': out << std::dec << static_cast<std::uint32_t>(extra_arg()); break;
            case '%': out.put('%'); break;
            default:  handle_unknown_fmt<index>(fmt[i]);
        }
    }
}

template <_Index index>
[[nodiscard]]
static auto checked_scanf_impl(
    RegisterFile &rf, Memory &mem, std::istream &in, std::string_view fmt, Register from
) -> std::pair<std::size_t, std::size_t> {
    auto reg             = reg_to_int(from);
    const auto extra_arg = [&]() {
        if (reg == reg_to_int(Register::a7) + 1)
            throw FailToInterpret{
                .error = Error::NotImplemented, .message = "Too many arguments for scanf"
            };
        return rf[int_to_reg(reg++)];
    };

    static std::string buf{};
    std::size_t args{};
    std::size_t io_count{};

    for (std::size_t i = 0; i < fmt.size(); ++i) {
        char c = fmt[i];
        if (c != '%') {
            ++io_count;
            if (in.get() != c)
                return {reg - reg_to_int(from), io_count};
            else
                continue;
        }

        // c == '%' here
        auto val_ch = char{};
        auto val_u  = target_size_t{};
        auto val_s  = target_ssize_t{};

        switch (fmt[++i]) {
            case 'd': {
                in >> val_s;
                char buffer[std::numeric_limits<int>::digits10 + 2];
                auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), val_s);
                io_count += ptr - buffer + 1;
                aligned_access<index, std::int32_t>(mem, extra_arg()) = val_s;
                break;
            }
            case 's': {
                in >> buf;
                io_count += buf.size();
                auto ptr = extra_arg();
                auto raw = checked_get_area<index>(mem, ptr, buf.size() + 1);
                std::memcpy(raw, buf.data(), buf.size() + 1);
                break;
            }
            case 'c': {
                in.get(val_ch); // don't skip whitespace
                ++io_count;
                aligned_access<index, char>(mem, extra_arg()) = val_ch;
                break;
            }
            case 'u': {
                in >> val_u;
                char buffer[std::numeric_limits<int>::digits10 + 2];
                auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), val_u);
                io_count += ptr - buffer + 1;
                aligned_access<index, std::uint32_t>(mem, extra_arg()) = val_u;
                break;
            }
            default: handle_unknown_fmt<index>(fmt[i]);
        }

        if (in.good()) {
            ++args;
        }
    }

    return {args, io_count};
}

auto puts(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    auto ptr = rf[Register::a0];
    auto str = checked_get_string<_Index::puts>(mem, ptr);
    dev.out << str << '\n';

    dev.counter.libcIO += kLibcOverhead + io(str.size() + 1);

    return return_to_user(rf, mem, 0);
}

auto putchar(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    auto c = rf[Register::a0];
    dev.out.put(static_cast<char>(c));
    dev.counter.libcIO += kLibcOverhead + io(1);
    return return_to_user(rf, mem, 0);
}

auto printf(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    auto ptr = rf[Register::a0];
    auto fmt = checked_get_string<_Index::printf>(mem, ptr);
    auto os  = std::stringstream{};
    checked_printf_impl<_Index::printf>(rf, mem, os, fmt, Register::a1);

    auto str = std::move(os).str();
    dev.out << str;

    dev.counter.libcIO += kLibcOverhead + io(str.size()) + op(fmt.size());

    return return_to_user(rf, mem, 0);
}

auto sprintf(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    auto ptr0 = rf[Register::a0];
    auto ptr1 = rf[Register::a1];
    auto fmt  = checked_get_string<_Index::sprintf>(mem, ptr1);

    auto os = std::stringstream{};
    checked_printf_impl<_Index::sprintf>(rf, mem, os, fmt, Register::a2);

    auto str = std::move(os).str();
    auto raw = checked_get_area<_Index::sprintf>(mem, ptr0, str.size() + 1);
    std::memcpy(raw, str.data(), str.size() + 1);

    // Format time + IO time
    dev.counter.libcOp += kLibcOverhead + io(str.size()) + op(fmt.size());

    return return_to_user(rf, mem, ptr0);
}

auto getchar(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    auto c = dev.in.get();
    dev.counter.libcIO += kLibcOverhead + io(1);
    return return_to_user(rf, mem, c);
}

auto scanf(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    auto ptr    = rf[Register::a0];
    auto fmt    = checked_get_string<_Index::scanf>(mem, ptr);
    auto &is    = dev.in;
    auto result = checked_scanf_impl<_Index::scanf>(rf, mem, is, fmt, Register::a1);

    dev.counter.libcIO += kLibcOverhead + io(result.second) + op(fmt.size());

    return return_to_user(rf, mem, result.first);
}

auto sscanf(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    auto ptr0 = rf[Register::a0];
    auto ptr1 = rf[Register::a1];
    auto str  = checked_get_string<_Index::sscanf>(mem, ptr0);
    auto fmt  = checked_get_string<_Index::sscanf>(mem, ptr1);

    auto is     = std::stringstream{std::string(str)};
    auto result = checked_scanf_impl<_Index::sscanf>(rf, mem, is, fmt, Register::a2);

    // Format time + IO time
    dev.counter.libcOp += kLibcOverhead + op(result.second) + op(fmt.size());

    return return_to_user(rf, mem, result.first);
}

} // namespace dark::libc::__details
