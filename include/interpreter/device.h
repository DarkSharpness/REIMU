#pragma once
#include <declarations.h>
#include <weight.h>
#include <iosfwd>

namespace dark {

struct Device {
    weight::counter counter {};
    std::size_t iparse_count{};

    std::istream &in;
    std::ostream &out;

    void predict(target_size_t pc, bool result);
};

} // namespace dark
