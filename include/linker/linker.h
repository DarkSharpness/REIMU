#pragma once
#include <declarations.h>
#include <map>
#include <span>
#include <vector>

namespace dark {

struct Linker {
    using _Storage_t = std::unique_ptr<dark::Storage>;

    // Offset of storage's start in the storage
    // We may assume that offset[0] = 0.
    // And offset.back() = next storage's start - storage's start
    struct StorageDetails {
        std::size_t absolute_offset {};
        std::vector <std::size_t> offsets;
        bool is_initialized() const { return !offsets.empty(); }
        std::size_t section_size() const { return offsets.capacity() - 1; }
    };

    using _Storage_Map_t = std::map <_Storage_t *, StorageDetails>;
    using _Section_Vec_t = std::vector <typename _Storage_Map_t::iterator>;
    static constexpr auto kSections = static_cast<std::size_t>(Section::MAXCOUNT);

    _Storage_Map_t storage_map;
    _Section_Vec_t section_vec[kSections];

    explicit Linker(std::span<Assembler>);
};

} // namespace dark
