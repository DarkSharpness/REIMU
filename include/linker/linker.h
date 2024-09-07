#pragma once
#include "declarations.h"
#include "utility/any.h"
#include <deque>
#include <memory>
#include <span>
#include <unordered_map>

namespace dark {

struct Storage;
struct MemoryLayout;
struct AssemblyLayout;

struct Linker {
public:
    using _Storage_t = std::unique_ptr<Storage>;
    using _Slice_t   = std::span<_Storage_t>;

    using LinkResult = MemoryLayout;
    struct SymbolLocation;
    struct StorageDetails;

    using _Details_Vec_t  = std::deque<StorageDetails>;
    using _Symbol_Table_t = std::unordered_map<std::string_view, SymbolLocation>;

    explicit Linker(std::span<AssemblyLayout>);

    [[nodiscard]] auto get_linked_layout() && -> LinkResult;

    /**
     * Location of the symbol in the storage
     * It contains of the pointer of absolute position in the output file
     * and the pointer of the offset in the storage.
     */
    struct SymbolLocation {
    public:
        explicit SymbolLocation(const StorageDetails &details, std::size_t index);
        auto get_location() const { return *absolute + *offset; }
        void next_offset() { ++offset; }

    protected:
        explicit SymbolLocation(const target_size_t *pos, const target_size_t *off) :
            absolute(pos), offset(off) {}

    private:
        const target_size_t *absolute;
        const target_size_t *offset;
    };

    /**
     * Storage details, including the span area of the storage.
     * We may assume that offset[0] = 0.
     * And offset.back() = next storage's start - storage's start
     */
    struct StorageDetails {
    public:
        explicit StorageDetails(_Slice_t storage, _Symbol_Table_t &table);

        struct Iterator {
            _Storage_t *storage;
            SymbolLocation location;
            friend bool operator==(const Iterator &lhs, const Iterator &rhs);
            Iterator &operator++();
            auto operator*() const -> std::pair<_Storage_t &, target_size_t> {
                return {*storage, location.get_location()};
            }
        };

        auto begin() const -> Iterator;
        auto end() const -> Iterator;

        auto get_start() const { return begin_position; }
        void set_start(target_size_t start) { begin_position = start; }
        auto *get_local_table() const { return table; }
        auto get_offsets() const -> std::span<target_size_t> {
            return {offsets.get(), storage.size() + 1};
        }

    private:
        friend struct SymbolLocation;
        _Slice_t storage;                         // Storage in the section
        target_size_t begin_position;             // Position in the output file
        std::unique_ptr<target_size_t[]> offsets; // Sizes of sections in the storage
        _Symbol_Table_t *table;                   // Local symbol table
    };

private:

    static constexpr auto kSections = static_cast<std::size_t>(Section::MAXCOUNT);

    _Details_Vec_t details_vec[kSections]; // Details of the sections
    _Symbol_Table_t global_symbol_table;   // Global symbol table

    any result; // Result of the linking

    void add_libc();
    void add_file(AssemblyLayout &file, _Symbol_Table_t &table);
    auto get_section(Section section) -> _Details_Vec_t &;
    void make_estimate();
    void make_relaxation();
    void link();
};

} // namespace dark
