#pragma once
#include <string>
#include <declarations.h>

namespace dark {

enum class Error {
    InsMisAligned,
    InsOutOfBound,
    InsUnknown,

    LoadMisAligned,
    LoadOutOfBound,
    StoreMisAligned,
    StoreOutOfBound,

    DivideByZero,

};

struct FailToInterpret {
    Error error;
    target_size_t arg0; // Address | Value
    target_size_t arg1; // Alignment | Value
};

} // namespace dark
