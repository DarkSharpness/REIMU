#pragma once
#include "declarations.h"
#include <vector>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <cstdint>

namespace dark {

/**
 * @brief Represents a collection of parts that are assembled together.
 * Supported part now:
 *  .text
 *  .data
 *  .sdata
 *  .rodata
 *  .srodata
 *  .bss
 *  .sbss
 *  .section
 *  .globl
 * 
 *  .align
 *  .p2align
 *  .balign
 * 
 *  .string
 *  .asciz
 *  .byte
 *  .2byte
 *  .half
 *  .short
 *  .4byte
 *  .long
 *  .word
 *  .type
 * Ignored attributes:
 *  .attribute
 *  .file
 *
 *  .size
 *  others
*/

struct Assembly {
    enum class Section : std::uint8_t {
        INVALID = 0, // Invalid section
        TEXT,   // Code
        DATA,   // Initialized data
        RODATA, // Read-only data
        BSS     // Initialized to zero
    } current_section {Section::INVALID};
    struct LabelData {
        std::size_t line_number {};
        std::size_t data_index  {};
        std::string_view label_name;
        bool    global  {};
        Section section {};
    };
    struct Storage {
        virtual ~Storage() noexcept = default;
    };

    std::unordered_map <std::string, LabelData> labels;
    std::vector <std::unique_ptr <Storage>> storages;

    std::string         file_info;      // Debug information
    std::size_t         line_number;    // Debug information

    explicit Assembly(std::string_view);

    [[noreturn]] void fail_to_parse();
    void add_label(std::string_view);
    void parse_line(std::string_view);
    void parse_command(std::string_view, std::string_view);
    void parse_storage(std::string_view, std::string_view);
};

} // namespace dark
