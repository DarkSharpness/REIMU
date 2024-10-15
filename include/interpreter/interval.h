#pragma once
#include "declarations.h"

namespace dark {

struct Interval {
    target_size_t start;
    target_size_t finish;
    bool contains(target_size_t lo, target_size_t hi) const { return start <= lo && hi <= finish; }
};

} // namespace dark
