#pragma once
#include <declarations.h>
#include <weight.h>
#include <iosfwd>
#include <memory>

namespace dark {

struct Device {
    struct Counter : weight::counter {
        std::size_t iparse;
    } counter;

    std::istream &in;
    std::ostream &out;

    static auto create(const Config &config) ->std::unique_ptr<Device>;

    // Predict a branch at pc. It will call external branch predictor
    void predict(target_size_t pc, bool result);

    ~Device();
  private:
    struct Impl;

    auto get_impl() -> Impl &;
};

} // namespace dark
