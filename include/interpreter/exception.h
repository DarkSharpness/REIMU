#pragma once
#include <string>
#include <declarations.h>

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

};

struct FailToInterpret {
    Error error;
    target_size_t arg0; // Address | Value
    target_size_t arg1; // Alignment | Value
};

} // namespace dark
