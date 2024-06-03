#pragma once
#include "declarations.h"
#include <string_view>
#include <vector>
#include <unordered_map>

namespace dark {

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