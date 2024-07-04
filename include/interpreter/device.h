#pragma once
#include <declarations.h>
#include <weight.h>
#include <any>
#include <iosfwd>
#include <memory>

namespace dark {

struct Device {
    struct Counter : weight::counter {
        std::size_t iparse;
    } counter;

    std::istream &in;
    std::ostream &out;

    std::any branch_predictor;

    static auto create(const Config &config, std::istream &in, std::ostream &out)
        ->std::unique_ptr<Device>;

    // Predict a branch at pc. It will call external branch predictor
    void predict(target_size_t pc, bool result);
};

} // namespace dark
