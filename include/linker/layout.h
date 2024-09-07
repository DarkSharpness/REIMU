#pragma once
#include "declarations.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace dark {

struct MemoryLayout {
    // A table that maps the symbol to its final position.
    std::unordered_map<std::string, target_size_t> position_table;

    // A table which indicates the storage at given position.
    // The vector should be 4 bytes aligned at least.
    struct Section {
        target_size_t start;
        std::vector<std::byte> storage;
        target_size_t begin() const { return start; }
        target_size_t end() const { return start + storage.size(); }
    };

    Section text;
    Section data;
    Section rodata;
    Section unknown;
    Section bss;
};

} // namespace dark
