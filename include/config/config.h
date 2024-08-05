#pragma once
#include <declarations.h>
#include <span>
#include <iosfwd>
#include <memory>
#include <string_view>

namespace dark {

struct Config {
  public:
    static auto parse(int argc, char **argv) -> std::unique_ptr <Config>;

    auto get_input_stream() const -> std::istream &;
    auto get_output_stream() const -> std::ostream &;

    auto get_stack_top() const -> target_size_t;
    auto get_stack_low() const -> target_size_t;
    auto get_timeout() const -> std::size_t;

    auto get_assembly_names() const -> std::span <const std::string_view>;

    auto enable_detail()    const -> bool;
    auto enable_debug()     const -> bool;

    auto has_option(std::string_view) const -> bool;
    auto get_weight(std::string_view) const -> std::size_t;

    ~Config();
  private:
    struct Impl;
    auto get_impl() const -> const Impl &;
};

} // namespace dark
