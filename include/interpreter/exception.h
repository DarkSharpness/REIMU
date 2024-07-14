#pragma once
#include <string>
#include <declarations.h>
#include <interpreter/forward.h>

namespace dark {

enum class Error {
    LoadMisAligned,
    LoadOutOfBound,
    StoreMisAligned,
    StoreOutOfBound,

    InsMisAligned,
    InsOutOfBound,
    InsUnknown,

    DivideByZero,

    NotImplemented,
};

struct FailToInterpret {
    Error error;
    union {
        target_size_t address;
    };
    union {
        target_size_t alignment;
    };

    auto what(RegisterFile &, Memory &, Device &) const -> std::string;
};

} // namespace dark
