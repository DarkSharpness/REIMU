#include <utility.h>
#include <libc/libc.h>
#include <libc/memory.h>
#include <libc/utility.h>
#include <interpreter/device.h>
#include <interpreter/memory.h>
#include <interpreter/register.h>
#include <interpreter/exception.h>
#include <cstring>
#include <sstream>

namespace dark::libc::__details {

[[maybe_unused, noreturn]]
static void throw_not_implemented() {
    throw FailToInterpret { .error = Error::NotImplemented, .message = "Not implemented" };
}

template <_Index index>
static auto checked_get_string(Memory &mem, target_size_t str, std::size_t extra = 0) {
    auto area = mem.libc_access(str);
    auto length = ::strnlen(area.data(), area.size());

    if (length + extra >= area.size())
        handle_outofbound<index>(str + area.size(), sizeof(char));

    return std::string_view(area.data(), length);
}

template <_Index index>
static auto checked_get_area(Memory &mem, target_size_t ptr, target_size_t size) {
    auto area = mem.libc_access(ptr);

    if (area.size() < size)
        handle_outofbound<index>(ptr + size, sizeof(char));

    return area.data();
}

template <_Index index>
static auto checked_get_areas(Memory &mem, target_size_t dst, target_size_t src, target_size_t size) {
    auto area0 = mem.libc_access(dst);
    auto area1 = mem.libc_access(src);

    if (area0.size() < area1.size()) {
        if (area0.size() < size)
            handle_outofbound<index>(dst + size, sizeof(char));
    } else {
        if (area1.size() < size)
            handle_outofbound<index>(src + size, sizeof(char));
    }

    return std::make_pair(area0.data(), area1.data());
}

static char __libc_io_fmt_error[] = "unknown format specifier: % ";

template <_Index index>
[[noreturn]]
static void handle_unknown_fmt(char what) {
    auto data = std::span(__libc_io_fmt_error);
    data.rbegin()[1] = what;
    throw FailToInterpret {
        .error      = Error::LibcError,
        .libc_which = static_cast<libc_index_t>(index),
        .message    = data.data()
    };
}

template <_Index index>
static void checked_printf_impl(
    RegisterFile &rf, Memory &mem, std::ostream &out,
    std::string_view str, Register from
) {
    auto reg = reg_to_int(from);
    const auto extra_arg = [&]() {
        if (reg == reg_to_int(Register::a7) + 1)
            throw FailToInterpret {
                .error = Error::NotImplemented,
                .message = "Too many arguments for (s)printf"
            };
        return rf[int_to_reg(reg++)];
    };

    for (std::size_t i = 0 ; i < str.size() ; ++i) {
        char c = str[i];
        if (c != '%') { out.put(c); continue; }
        // c == '%' here
        switch (str[++i]) {
            case 'd':
                out << std::dec << static_cast<std::int32_t>(extra_arg());
                break;
            case 's':
                out << checked_get_string<_Index::printf>(mem, extra_arg());
                break;
            case 'c':
                out.put(static_cast<char>(extra_arg()));
                break;
            case 'x':
                out << std::hex << extra_arg();
                break;
            case 'p':
                out << "0x" << std::hex << extra_arg();
                break;
            case 'u':
                out << std::dec << static_cast<std::uint32_t>(extra_arg());
                break;
            case '%':
                out.put('%');
                break;
            default:
                handle_unknown_fmt<index>(str[i]);
        }
    }
}

template <_Index index>
[[nodiscard]]
static auto checked_scanf_impl(
    RegisterFile &rf, Memory &mem, std::istream &in,
    std::string_view str, Register from
) -> int {
    auto reg = reg_to_int(from);
    const auto extra_arg = [&]() {
        if (reg == reg_to_int(Register::a7) + 1)
            throw FailToInterpret {
                .error = Error::NotImplemented,
                .message = "Too many arguments for (s)printf"
            };
        return rf[int_to_reg(reg++)];
    };

    static std::string buf {};

    for (std::size_t i = 0 ; i < str.size() ; ++i) {
        char c = str[i];
        if (c != '%') { 
            if (in.get() != c)
                return reg - reg_to_int(from);
            else
                continue;
        }

        // c == '%' here

        auto val_ch = target_size_t {};
        auto val_u  = target_size_t {};
        auto val_s  = target_ssize_t {};

        switch (str[++i]) {
            case 'd':
                in >> val_s;
                aligned_access<index, std::int32_t>(mem, extra_arg()) = val_s;
                break;
            case 's': {
                in >> buf;
                auto ptr = extra_arg();
                auto raw = checked_get_area <index> (mem, ptr, buf.size() + 1);
                std::memcpy(raw, buf.data(), buf.size() + 1);
                break;
            }
            case 'c':
                in >> val_ch;
                aligned_access<index, char>(mem, extra_arg()) = val_ch;
                break;
            default:
                handle_unknown_fmt<index>(str[i]);
        }
    }

    return 0;
}


void puts(Executable &, RegisterFile &rf, Memory &mem, Device &dev) {
    auto ptr = rf[Register::a0];
    auto str = checked_get_string<_Index::puts>(mem, ptr);
    dev.out << str << '\n';
    return_to_user(rf, mem, 0);
}

void putchar(Executable &, RegisterFile &rf, Memory &mem, Device &dev) {
    auto c = rf[Register::a0];
    dev.out.put(static_cast<char>(c));
    return_to_user(rf, mem, 0);
}

void printf(Executable &, RegisterFile &rf, Memory &mem, Device &dev) {
    auto ptr = rf[Register::a0];
    auto fmt = checked_get_string<_Index::printf>(mem, ptr);
    auto &os = dev.out;

    checked_printf_impl <_Index::printf> (rf, mem, os, fmt, Register::a1);
    return_to_user(rf, mem, 0);
}

void sprintf(Executable &, RegisterFile &rf, Memory &mem, Device &) {
    auto ptr0 = rf[Register::a0];
    auto ptr1 = rf[Register::a1];
    auto fmt  = checked_get_string<_Index::sprintf>(mem, ptr1);

    std::stringstream ss;
    checked_printf_impl <_Index::sprintf> (rf, mem, ss, fmt, Register::a2);

    auto str  = std::move(ss).str();
    auto raw  = checked_get_area<_Index::sprintf>(mem, ptr0, str.size() + 1);

    std::memcpy(raw, str.data(), str.size() + 1);
    return_to_user(rf, mem, ptr0);
}

void getchar(Executable &, RegisterFile &rf, Memory &mem, Device &dev) {
    auto c = dev.in.get();
    return_to_user(rf, mem, c);
}

void scanf(Executable &, RegisterFile &rf, Memory &mem, Device &dev) {
    auto ptr = rf[Register::a0];
    auto str = checked_get_string<_Index::scanf>(mem, ptr);
    auto &is = dev.in;

    auto result = checked_scanf_impl <_Index::scanf> (rf, mem, is, str, Register::a1);
    return_to_user(rf, mem, result);
}

void sscanf(Executable &, RegisterFile &rf, Memory &mem, Device &) {
    auto ptr0 = rf[Register::a0];
    auto ptr1 = rf[Register::a1];
    auto str  = checked_get_string<_Index::sscanf>(mem, ptr1);

    std::stringstream ss { std::string(str) };

    auto result = checked_scanf_impl <_Index::sscanf> (rf, mem, ss, str, Register::a2);
    return_to_user(rf, mem, result);
}

void malloc(Executable &, RegisterFile &rf, Memory &mem, Device &) {
    auto size = rf[Register::a0];
    auto [_, retval] = malloc_manager.allocate(mem, size);
    return_to_user(rf, mem, retval);
}

void calloc(Executable &, RegisterFile &rf, Memory &mem, Device &) {
    auto size = rf[Register::a0] * rf[Register::a1];
    auto [ptr, retval] = malloc_manager.allocate(mem, size);
    std::memset(ptr, 0, size);
    return_to_user(rf, mem, retval);
}

void realloc(Executable &, RegisterFile &rf, Memory &mem, Device &) {
    auto new_size = rf[Register::a1];
    malloc_manager.free(mem, rf[Register::a0]);
    auto [_, retval] = malloc_manager.allocate(mem, new_size);
    return_to_user(rf, mem, retval);
}

void free(Executable &, RegisterFile &rf, Memory &mem, Device &) {
    malloc_manager.free(mem, rf[Register::a0]);
    return_to_user(rf, mem, 0);
}

void memset(Executable &, RegisterFile &rf, Memory &mem, Device &) {
    auto ptr  = rf[Register::a0];
    auto fill = rf[Register::a1];
    auto size = rf[Register::a2];
    auto raw  = checked_get_area<_Index::memset>(mem, ptr, size);
    std::memset(raw, fill, size);
    return_to_user(rf, mem, ptr);
}

void memcmp(Executable &, RegisterFile &rf, Memory &mem, Device &) {
    auto ptr0 = rf[Register::a0];
    auto ptr1 = rf[Register::a1];
    auto size = rf[Register::a2];

    auto [lhs, rhs] = checked_get_areas<_Index::memcmp>(mem, ptr0, ptr1, size);

    auto result = std::memcmp(lhs, rhs, size);
    return_to_user(rf, mem, result);
}

void memcpy(Executable &, RegisterFile &rf, Memory &mem, Device &) {
    auto ptr0 = rf[Register::a0];
    auto ptr1 = rf[Register::a1];
    auto size = rf[Register::a2];

    auto [dst, src] = checked_get_areas<_Index::memcpy>(mem, ptr0, ptr1, size);

    std::memcpy(dst, src, size);
    return_to_user(rf, mem, ptr0);
}

void memmove(Executable &, RegisterFile &rf, Memory &mem, Device &) {
    auto ptr0 = rf[Register::a0];
    auto ptr1 = rf[Register::a1];
    auto size = rf[Register::a2];

    auto [dst, src] = checked_get_areas<_Index::memmove>(mem, ptr0, ptr1, size);

    std::memmove(dst, src, size);
    return_to_user(rf, mem, ptr0);
}

void strcpy(Executable &, RegisterFile &rf, Memory &mem, Device &) {
    auto ptr0 = rf[Register::a0];
    auto ptr1 = rf[Register::a1];

    auto str  = checked_get_string<_Index::strcpy>(mem, ptr1);
    auto size = str.size() + 1;
    auto raw  = checked_get_area<_Index::strcpy>(mem, ptr0, size);

    std::memcpy(raw, str.data(), size);
    return_to_user(rf, mem, ptr0);
}

void strlen(Executable &, RegisterFile &rf, Memory &mem, Device &) {
    auto ptr = rf[Register::a0];
    auto str = checked_get_string<_Index::strlen>(mem, ptr);
    return_to_user(rf, mem, str.size());
}

void strcat(Executable &, RegisterFile &rf, Memory &mem, Device &) {
    auto ptr0 = rf[Register::a0];
    auto ptr1 = rf[Register::a1];

    auto str1 = checked_get_string<_Index::strcat>(mem, ptr1);
    auto str0 = checked_get_string<_Index::strcat>(mem, ptr0, str1.size());
    auto raw  = const_cast<char *>(str0.end());

    // Copy the string and the null terminator
    std::memcpy(raw, str1.data(), str1.size() + 1);
    return_to_user(rf, mem, ptr0);
}

void strcmp(Executable &, RegisterFile &rf, Memory &mem, Device &) {
    auto ptr0 = rf[Register::a0];
    auto ptr1 = rf[Register::a1];

    auto str0 = checked_get_string<_Index::strcmp>(mem, ptr0);
    auto str1 = checked_get_string<_Index::strcmp>(mem, ptr1);

    auto result = std::strcmp(str0.data(), str1.data());
    return_to_user(rf, mem, result);
}

} // namespace dark::libc::__details
