#pragma once

// #include "common.h"
#include <vector>
#include <ranges>
#include <iostream>
#include <string_view>
#include <source_location>
#include <unordered_map>
#include <format>

namespace dark {

template <typename _Tp, typename ..._Args>
void panic_if(_Tp &&condition, std::format_string <_Args...> fmt, _Args &&...args) {
    if (!condition) return;
    // Failure case, print the message and exit
    std::cerr << std::format(fmt, std::forward <_Args>(args)...) << std::endl;
    std::exit(EXIT_FAILURE);
}

/**
 * @brief Runtime assertion, if the condition is false, print the message and exit.
 * @param condition Assertion condition.
 */
template <typename _Tp, typename ...Args>
void runtime_assert(_Tp &&condition, std::source_location where = std::source_location::current()) {
    if (condition) return;
    // Failure case, print the message and exit
    std::cerr << std::format(
        "Assertion failed at {}:{} in {}:\n"
        "Internal error, please report this issue to the developer.\n",
        where.file_name(), where.line(), where.function_name());
    std::exit(EXIT_FAILURE);
}

/**
 * @brief Split the string_view by the given delimiter
 * @param view Input string.
 * @param policy Policy to find the delimiters.
 * @return A vector of subranges of the input string.
 */
template <typename _Delim_Policy_t>
auto split_string(std::string_view view, _Delim_Policy_t &&policy) -> std::vector <std::string_view> {
    // Skip the prefix delimiters, and return the reference to the view.
    auto &&skip_delimiter = [&policy](std::string_view &view) -> std::string_view & {
        auto pos = std::ranges::find_if_not(view, policy);
        return view = view.substr(pos - view.begin());
    };

    std::vector <std::string_view> result;

    while (!skip_delimiter(view).empty()) {
        auto pos = std::ranges::find_if(view, policy) - view.begin();
        result.push_back(view.substr(0, pos));
        view.remove_prefix(pos);
    }

    return result;
}

struct Config {
    std::string_view input_file;                    // Input file
    std::string_view output_file;                   // Output file
    std::vector <std::string_view> assembly_files;  // Assembly files

    inline static constexpr std::size_t uninitialized = std::size_t(-1);

    std::size_t storage_size = uninitialized;     // Memory storage 
    std::size_t maximum_time = uninitialized;     // Maximum time

    // The additional configuration table provided by the user.
    std::unordered_map <std::string_view, bool> option_table;
    // The additional weight table provided by the user.
    std::unordered_map <std::string_view, std::size_t> weight_table;

    static auto parse(int argc, char **argv) -> Config;
    void initialize_with_check();
    void print_in_detail() const;
};


} // namespace dark

