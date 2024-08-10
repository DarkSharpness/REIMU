#pragma once
#include <declarations.h>
#include <utility/deleter.h>
#include <config/counter.h>
#include <iosfwd>

namespace dark {

struct Device {
  public:
    using unique_t = derival_ptr<Device>;

    struct Counter : config::Counter {
        std::size_t iparse;
    } counter;

    std::istream &in;
    std::ostream &out;

    static auto create(const Config &config) -> unique_t;
    // Predict a branch at pc. It will call external branch predictor
    void predict(target_size_t pc, bool result);
    // Print in details
    void print_details(bool) const;

  private:
    struct Impl;

    auto get_impl() -> Impl &;
};

} // namespace dark
