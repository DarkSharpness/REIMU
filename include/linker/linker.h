#pragma once
#include <declarations.h>
#include <map>
#include <span>
#include <deque>
#include <vector>
#include <memory>
#include <unordered_map>

namespace dark {

struct Linker {
    using _Storage_t = std::unique_ptr<dark::Storage>;

    struct SymbolLocation;
    struct StorageDetails;

    using _Details_Vec_t    = std::deque<StorageDetails>;
    using _Symbol_Table_t   = std::unordered_map<std::string_view, SymbolLocation>;

    /**
     * Storage details, including the span area of the storage.
     * We may assume that offset[0] = 0.
     * And offset.back() = next storage's start - storage's start
     */
    struct StorageDetails {
        std::span<_Storage_t> storage;          // Storage in the section
        std::size_t begin_position;             // Position in the output file
        std::unique_ptr<std::size_t[]> offsets; // Sizes of sections in the storage
        _Symbol_Table_t *table;                 // Local symbol table

        explicit StorageDetails(std::span<_Storage_t> storage, _Symbol_Table_t &table)
            : storage(storage), begin_position(),
              offsets(std::make_unique<std::size_t[]>(storage.size() + 1)), table(&table) {}
    };

    /**
     * Location of the symbol in the storage
     * It contains of the pointer of absolute position in the output file
     * and the pointer of the offset in the storage.
     */
    struct SymbolLocation {
        std::size_t *position;
        std::size_t *offset;

        explicit SymbolLocation(StorageDetails &details, std::size_t index)
            : position(&details.begin_position), offset(&details.offsets[index]) {}

        auto query_position() const { return std::make_pair(*position, *offset); }
    };

    static constexpr auto kSections = static_cast<std::size_t>(Section::MAXCOUNT);

    _Details_Vec_t details_vec[kSections];
    _Symbol_Table_t global_symbol_table;

    explicit Linker(std::span<Assembler>);

  private:
    void add_file(Assembler &file, _Symbol_Table_t &table);
    auto get_section(Section section) -> _Details_Vec_t &;
    void make_estimate();
    void make_relaxation();
};

} // namespace dark
