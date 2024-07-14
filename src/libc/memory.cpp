#include <utility.h>
#include <libc/libc.h>
#include <libc/memory.h>
#include <libc/utility.h>
#include <interpreter/device.h>
#include <interpreter/memory.h>
#include <interpreter/register.h>

namespace dark::libc::__details {

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

} // namespace dark::libc::__details
