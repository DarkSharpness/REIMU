#pragma once
#include <declarations.h>
#include <utility/deleter.h>
#include <config/counter.h>
#include <iosfwd>

namespace dark {

struct Device {
public:
    using unique_t = derival_ptr<Device>;

    struct Counter : weight::Counter {
        std::size_t iparse;
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
