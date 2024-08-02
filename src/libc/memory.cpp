#include <utility.h>
#include <libc/libc.h>
#include <libc/memory.h>
#include <libc/utility.h>
#include <interpreter/device.h>
#include <interpreter/memory.h>
#include <interpreter/register.h>

namespace dark::libc {

static MemoryManager malloc_manager {};

void libc_init(RegisterFile &, Memory &mem, Device &) {
    malloc_manager.init(mem);
}

void MemoryManager::unknown_malloc_pointer(target_size_t ptr, __details::_Index index) {
    throw FailToInterpret {
        .error      = Error::LibcError,
        .libc_which = static_cast<libc_index_t>(index),
        .message    = std::format("not a malloc pointer: {:#x}", ptr),
    };
}

} // namespace dark::libc

namespace dark::libc::__details {

auto malloc(Executable &, RegisterFile &rf, Memory &mem, Device &) -> Hint {
    auto size = rf[Register::a0];
    auto [_, retval] = malloc_manager.allocate(mem, size);
    return return_to_user(rf, mem, retval);
}

auto calloc(Executable &, RegisterFile &rf, Memory &mem, Device &) -> Hint {
    auto size = rf[Register::a0] * rf[Register::a1];
    auto [ptr, retval] = malloc_manager.allocate(mem, size);
    std::memset(ptr, 0, size);
    return return_to_user(rf, mem, retval);
}

auto realloc(Executable &, RegisterFile &rf, Memory &mem, Device &) -> Hint {
    auto old_data = rf[Register::a0];
    auto new_size = rf[Register::a1];
    auto retval = malloc_manager.reallocate(mem, old_data, new_size);
    return return_to_user(rf, mem, retval);
}

auto free(Executable &, RegisterFile &rf, Memory &mem, Device &) -> Hint {
    malloc_manager.free(mem, rf[Register::a0]);
    return return_to_user(rf, mem, 0);
}

auto memcmp(Executable &, RegisterFile &rf, Memory &mem, Device &) -> Hint {
    auto ptr0 = rf[Register::a0];
    auto ptr1 = rf[Register::a1];
    auto size = rf[Register::a2];

    auto [lhs, rhs] = checked_get_areas<_Index::memcmp>(mem, ptr0, ptr1, size);

    auto result = std::memcmp(lhs, rhs, size);
    return return_to_user(rf, mem, result);
}

auto memcpy(Executable &, RegisterFile &rf, Memory &mem, Device &) -> Hint {
    auto ptr0 = rf[Register::a0];
    auto ptr1 = rf[Register::a1];
    auto size = rf[Register::a2];

    auto [dst, src] = checked_get_areas<_Index::memcpy>(mem, ptr0, ptr1, size);

    std::memcpy(dst, src, size);
    return return_to_user(rf, mem, ptr0);
}

auto memmove(Executable &, RegisterFile &rf, Memory &mem, Device &) -> Hint {
    auto ptr0 = rf[Register::a0];
    auto ptr1 = rf[Register::a1];
    auto size = rf[Register::a2];

    auto [dst, src] = checked_get_areas<_Index::memmove>(mem, ptr0, ptr1, size);

    std::memmove(dst, src, size);
    return return_to_user(rf, mem, ptr0);
}

} // namespace dark::libc::__details
