#include "interpreter/memory.h"
#include "declarations.h"
#include "interpreter/device.h"
#include "interpreter/register.h"
#include "libc/libc.h"
#include "libc/memory.h"
#include "libc/utility.h"

namespace dark::libc {

static MemoryManager malloc_manager{};

void libc_init(RegisterFile &, Memory &mem, Device &) {
    malloc_manager.init(mem);
}

void MemoryManager::unknown_malloc_pointer(target_size_t ptr, __details::_Index index) {
    throw FailToInterpret{
        .error      = Error::LibcError,
        .libc_which = static_cast<libc_index_t>(index),
        .message    = fmt::format("Not a malloc pointer: 0x{:#x}", ptr),
    };
}

} // namespace dark::libc

namespace dark::libc::__details {

auto malloc(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    auto size        = rf[Register::a0];
    auto [_, retval] = malloc_manager.allocate(mem, size);

    dev.counter.libcMem += malloc_manager.get_malloc_time(size);

    return return_to_user(rf, mem, retval);
}

auto calloc(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    auto size          = rf[Register::a0] * rf[Register::a1];
    auto [ptr, retval] = malloc_manager.allocate(mem, size);
    std::memset(ptr, 0, size);

    dev.counter.libcMem += malloc_manager.get_malloc_time(size) + op(size);

    return return_to_user(rf, mem, retval);
}

auto realloc(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    auto old_data          = rf[Register::a0];
    auto new_size          = rf[Register::a1];
    auto [retval, realloc] = malloc_manager.reallocate(mem, old_data, new_size);

    dev.counter.libcMem += malloc_manager.get_realloc_time(new_size, realloc);

    return return_to_user(rf, mem, retval);
}

auto free(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    malloc_manager.free(mem, rf[Register::a0]);
    dev.counter.libcMem += malloc_manager.get_free_time();
    return return_to_user(rf, mem, 0);
}

auto memcmp(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    auto ptr0       = rf[Register::a0];
    auto ptr1       = rf[Register::a1];
    auto size       = rf[Register::a2];
    auto [lhs, rhs] = checked_get_areas<_Index::memcmp>(mem, ptr0, ptr1, size);

    auto pos = find_first_diff(lhs, rhs, size);
    dev.counter.libcOp += kLibcOverhead + op(pos * 2);

    auto result = std::memcmp(lhs, rhs, size);
    return return_to_user(rf, mem, result);
}

auto memcpy(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    auto ptr0       = rf[Register::a0];
    auto ptr1       = rf[Register::a1];
    auto size       = rf[Register::a2];
    auto [dst, src] = checked_get_areas<_Index::memcpy>(mem, ptr0, ptr1, size);
    std::memcpy(dst, src, size);

    dev.counter.libcOp += kLibcOverhead + op(size * 2);

    return return_to_user(rf, mem, ptr0);
}

auto memmove(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    auto ptr0       = rf[Register::a0];
    auto ptr1       = rf[Register::a1];
    auto size       = rf[Register::a2];
    auto [dst, src] = checked_get_areas<_Index::memmove>(mem, ptr0, ptr1, size);
    std::memmove(dst, src, size);

    dev.counter.libcOp += kLibcOverhead + op(size * 2);

    return return_to_user(rf, mem, ptr0);
}

auto memset(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    auto ptr  = rf[Register::a0];
    auto fill = rf[Register::a1];
    auto size = rf[Register::a2];
    auto raw  = checked_get_area<_Index::memset>(mem, ptr, size);
    std::memset(raw, fill, size);

    dev.counter.libcOp += kLibcOverhead + op(size);

    return return_to_user(rf, mem, ptr);
}

} // namespace dark::libc::__details
