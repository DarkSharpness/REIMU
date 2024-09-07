#pragma once
#include "config/counter.h"
#include "declarations.h"
#include "utility/deleter.h"
#include <cstddef>
#include <iosfwd>

namespace dark {

struct Device {
public:
    using unique_t = derival_ptr<Device>;

    struct Pair {
        std::size_t count;
        std::size_t weight;
        void operator+=(std::size_t w) {
            count++;
            weight += w;
        }
    };

    struct Counter : weight::Counter {
        std::size_t iparse;
        Pair libcMem; // malloc, free, calloc, realloc
        Pair libcIO;  // printf, scanf ...
        Pair libcOp;  // other libc functions
    } counter;

    std::istream &in;
    std::ostream &out;

    static auto create(const Config &config) -> unique_t;
    void predict(target_size_t pc, bool result);
    void print_details(bool) const;

    void try_load(target_size_t low, target_size_t size);
    void try_store(target_size_t low, target_size_t size);

private:
    struct Impl;

    auto get_impl() -> Impl &;
};

} // namespace dark
