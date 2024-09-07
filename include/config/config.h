#pragma once
#include "declarations.h"
#include "utility/deleter.h"
#include <iosfwd>
#include <span>
#include <string_view>

namespace dark {

namespace weight {
struct Counter;
} // namespace weight

struct Config {
public:
    using unique_t = derival_ptr<Config>;
    static auto parse(int argc, char **argv) -> unique_t;

    auto get_input_stream() const -> std::istream &;
    auto get_output_stream() const -> std::ostream &;

    auto get_stack_top() const -> target_size_t;
    auto get_stack_low() const -> target_size_t;
    auto get_timeout() const -> std::size_t;

    auto get_assembly_names() const -> std::span<const std::string_view>;

    auto has_option(std::string_view) const -> bool;
    auto get_weight() const -> const weight::Counter &;

private:
    struct Impl;
    auto get_impl() const -> const Impl &;
};

} // namespace dark
