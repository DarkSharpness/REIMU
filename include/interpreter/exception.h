#pragma once
#include <string>
#include <declarations.h>
#include <libc/forward.h>
#include <interpreter/forward.h>

namespace dark {

enum class Error : std::uint8_t {
    LoadMisAligned,
    LoadOutOfBound,

    StoreMisAligned,
    StoreOutOfBound,

    InsMisAligned,
    InsOutOfBound,
    InsUnknown,

    LibcMisAligned, // libc read/write access
    LibcOutOfBound, // libc read/write access

    DivideByZero,

    NotImplemented,
};

struct FailToInterpret {
    Error error;
    libc::libc_index_t libc_which;
    union {
        target_size_t address;
    };
    union {
        command_size_t  command;
        target_size_t   alignment;
        target_size_t   size;
    };

    auto what(RegisterFile &, Memory &, Device &) const -> std::string;
};

} // namespace dark
