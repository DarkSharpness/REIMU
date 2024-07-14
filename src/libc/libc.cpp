#include <utility.h>
#include <libc/libc.h>
#include <interpreter/device.h>
#include <interpreter/memory.h>
#include <interpreter/register.h>
#include <interpreter/exception.h>
#include <interpreter/executable.h>
#include <cstring>

namespace dark::libc::__details {

[[maybe_unused, noreturn]]
static void throw_not_implemented() {
    throw FailToInterpret { .error = Error::NotImplemented };
}

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

    auto span = mem.libc_access(addr);

    if (span.size() < sizeof(_Int))
        handle_outofbound<which>(addr, sizeof(_Int));

    // use launder + bit_cast to avoid undefined behavior
    return *std::bit_cast<_Int *>(span.data());
}

static void return_to_user(RegisterFile &rf, Memory &mem, target_size_t retval) {
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


void puts(Executable &, RegisterFile &rf, Memory &mem, Device &dev) {
    using enum Register;

    auto span   = mem.libc_access(rf[a0]);
    auto length = ::strnlen(span.data(), span.size());

    if (length == span.size())
        handle_outofbound<_Index::puts>(rf[a0] + length, sizeof(char));

    dev.out << std::string_view(span.data(), length) << '\n';

    return_to_user(rf, mem, 0);
}

void putchar(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void printf(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void sprintf(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void getchar(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void scanf(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void sscanf(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void malloc(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void calloc(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void realloc(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void free(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void memset(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void memcmp(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void memcpy(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void memmove(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void strcpy(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void strlen(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void strcat(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void strcmp(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

void strncmp(Executable &, RegisterFile &, Memory &, Device &) {
    throw_not_implemented();
}

} // namespace dark::libc
