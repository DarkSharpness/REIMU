#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace dark {

struct MemoryLayout {
    // A table that maps the symbol to its final position.
    std::unordered_map <std::string, std::size_t> position_table;

    // A table which indicates the storage at given position.
    // The vector should be 4 bytes aligned at least.
    struct Section {
        std::size_t start;
        std::vector <std::byte> storage;
        std::size_t begin() const { return start; }
        std::size_t end() const { return start + storage.size(); }
    };

    Section text;
    Section data;
    Section rodata;
    Section unknown;
    Section bss;
};

} // namespace dark
