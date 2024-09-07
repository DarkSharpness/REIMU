#include "interpreter/device.h"
#include "interpreter/memory.h"
#include "interpreter/register.h"
#include "libc/libc.h"
#include "libc/utility.h"
#include <cstring>
#include <string_view>

namespace dark::libc::__details {

auto strcpy(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    auto ptr0 = rf[Register::a0];
    auto ptr1 = rf[Register::a1];

    auto str  = checked_get_string<_Index::strcpy>(mem, ptr1);
    auto size = str.size() + 1;
    auto raw  = checked_get_area<_Index::strcpy>(mem, ptr0, size);

    std::memcpy(raw, str.data(), size);

    // strlen + memcpy
    dev.counter.libcOp += kLibcOverhead + op(size * 3);
    return return_to_user(rf, mem, ptr0);
}

auto strlen(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    auto ptr = rf[Register::a0];
    auto str = checked_get_string<_Index::strlen>(mem, ptr);

    dev.counter.libcOp += kLibcOverhead + op(str.size());
    return return_to_user(rf, mem, str.size());
}

auto strcat(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    auto ptr0 = rf[Register::a0];
    auto ptr1 = rf[Register::a1];

    auto str1 = checked_get_string<_Index::strcat>(mem, ptr1);
    auto str0 = checked_get_string<_Index::strcat>(mem, ptr0, str1.size());
    auto raw  = const_cast<char *>(str0.end());

    // Copy the string and the null terminator
    std::memcpy(raw, str1.data(), str1.size() + 1);

    // strlen + strcpy
    dev.counter.libcOp += kLibcOverhead + op(str0.size()) + op(str1.size() * 3);
    return return_to_user(rf, mem, ptr0);
}

auto strcmp(Executable &, RegisterFile &rf, Memory &mem, Device &dev) -> Hint {
    auto ptr0 = rf[Register::a0];
    auto ptr1 = rf[Register::a1];

    auto str0 = checked_get_string<_Index::strcmp>(mem, ptr0);
    auto str1 = checked_get_string<_Index::strcmp>(mem, ptr1);

    // Find which is different
    auto pos = find_first_diff(str0.data(), str1.data(), std::min(str0.size(), str1.size()));

    // strlen + memcmp
    dev.counter.libcOp += kLibcOverhead + op(pos * 4);

    auto result = std::strcmp(str0.data(), str1.data());

    return return_to_user(rf, mem, result);
}

} // namespace dark::libc::__details
