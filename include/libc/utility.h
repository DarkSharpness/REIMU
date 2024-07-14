// Should only be included once in libc.cpp
#include <libc/libc.h>
#include <interpreter/memory.h>
#include <interpreter/register.h>
#include <interpreter/exception.h>

namespace dark::libc::__details {

template <_Index which>
[[noreturn]]
static void handle_outofbound(target_size_t addr, target_size_t size) {
    throw FailToInterpret {
        .error      = Error::LibcOutOfBound,
        .libc_which = static_cast <libc_index_t> (which),
        .address    = addr,
        .size       = size,
    };
}

template <_Index which>
[[noreturn]]
static void handle_misaligned(target_size_t addr, target_size_t alignment) {
    throw FailToInterpret {
        .error      = Error::LibcMisAligned,
        .libc_which = static_cast <libc_index_t> (which),
        .address    = addr,
        .alignment  = alignment,
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

static void return_to_user(RegisterFile &rf, Memory &, target_size_t retval) {
    using enum Register;

    // Necessary setup
    rf[a0] = retval;
    rf.set_pc(rf[ra]);

    // Force the caller to comply to the calling convention
    constexpr Register caller_saved_poison[] =  {
        t0, t1, t2, t3, t4, t5, t6,
        a1, a2, a3, a4, a5, a6, a7,
        // a0 is the return value, which should not be poisoned
        // ra is the return address, normally it is untouched
    };

    // Enjoy the magic number ~
    for (auto reg : caller_saved_poison) rf[reg] = 0xDEADBEEF;
}

} // namespace dark::libc::__details
