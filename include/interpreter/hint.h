#pragma once
#include "interpreter/forward.h"

namespace dark {

/** A hint of what to execute next. */
struct Hint {
    Executable *next                         = nullptr;
    bool operator==(const Hint &other) const = default;
};


} // namespace dark
