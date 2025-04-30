#pragma once
#include "declarations.h"
#include "fmtlib.h"
#include "interpreter/exception.h"
#include "interpreter/hint.h"
#include "interpreter/memory.h"
#include "interpreter/register.h"
#include "libc/libc.h"
#include <cstddef>
#include <cstring>

namespace dark::libc::__details {

[[maybe_unused, noreturn]]
static void throw_not_implemented() {
    throw FailToInterpret{.error = Error::NotImplemented, .message = "Not implemented"};
}

template <_Index which>
[[noreturn]]
static void handle_outofbound(target_size_t addr, target_size_t size) {
    throw FailToInterpret{
        .error      = Error::LibcOutOfBound,
        .libc_which = static_cast<libc_index_t>(which),
        .detail =
            {
                .address = addr,
                .size    = size,
            }
    };
}

template <_Index which>
[[noreturn]]
static void handle_misaligned(target_size_t addr, target_size_t alignment) {
    throw FailToInterpret{
        .error      = Error::LibcMisAligned,
        .libc_which = static_cast<libc_index_t>(which),
        .detail =
            {
                .address   = addr,
                .alignment = alignment,
            }
    };
}


template <_Index index>
[[noreturn]]
static void handle_unknown_fmt(char what) {
    throw FailToInterpret{
        .error      = Error::LibcError,
        .libc_which = static_cast<libc_index_t>(index),
        .message    = fmt::format("nknown format specifier: %{}", what)
    };
}

template <_Index which, std::integral _Int>
static _Int &aligned_access(Memory &mem, target_size_t addr) {
    if (addr % alignof(_Int) != 0)
        handle_misaligned<which>(addr, sizeof(_Int));

    auto area = mem.libc_access(addr);

    if (area.size() < sizeof(_Int))
        handle_outofbound<which>(addr, sizeof(_Int));

    // use launder + bit_cast to avoid undefined behavior
    return *std::bit_cast<_Int *>(area.data());
}

template <_Index index>
static auto checked_get_string(Memory &mem, target_size_t str, std::size_t extra = 0) {
    auto area   = mem.libc_access(str);
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
static auto
checked_get_areas(Memory &mem, target_size_t dst, target_size_t src, target_size_t size) {
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

[[maybe_unused]]
static auto return_to_user(RegisterFile &rf, Memory &, target_size_t retval) -> Hint {
    using enum Register;

    // Necessary setup
    rf[a0] = retval;
    rf.set_pc(rf[ra]);

    // Force the caller to comply to the calling convention
    constexpr Register caller_saved_poison[] = {
        t0, t1, t2, t3, t4, t5, t6, a1, a2, a3, a4, a5, a6, a7,
        // a0 is the return value, which should not be poisoned
        // ra is the return address, normally it is untouched
    };

    // Enjoy the magic number ~
    for (auto reg : caller_saved_poison)
        rf[reg] = 0xDEADBEEF;

    return Hint{}; // No hint
}

static constexpr std::size_t kLibcOverhead = 32;

static constexpr auto io(target_size_t size) -> std::size_t {
    return 8 * size;
}
static constexpr auto op(target_size_t size) -> std::size_t {
    return 1 * size;
}

static constexpr auto find_first_diff(const char *lhs, const char *rhs, std::size_t size)
    -> std::size_t {
    for (std::size_t i = 0; i < size; ++i)
        if (lhs[i] != rhs[i])
            return i;
    return size;
}

} // namespace dark::libc::__details
