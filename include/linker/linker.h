#pragma once
#include <declarations.h>
#include <map>
#include <span>
#include <deque>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>

namespace dark {

struct Linker {
  public:
    using _Storage_t = std::unique_ptr<Storage>;
    using _Slice_t  = std::span<_Storage_t>;

    struct LinkResult;
    struct SymbolLocation;
    struct StorageDetails;

    using _Details_Vec_t    = std::deque<StorageDetails>;
    using _Symbol_Table_t   = std::unordered_map<std::string_view, SymbolLocation>;

    explicit Linker(std::span<Assembler>);

    const auto &get_result() { return *result; }

    struct LinkResult {
        // A table that maps the symbol to its final position.
        std::unordered_map <std::string_view, std::size_t> position_table;
        // A table which indicates the storage at given position.
        // The vector should be 4 bytes aligned at least.
        struct Section {
            std::size_t start;
            std::vector <std::byte> storage;
        };

        Section text;
        Section data;
        Section rodata;
        Section unknown;
        Section bss;
    };

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
        explicit SymbolLocation(const std::size_t *pos, const std::size_t *off)
            : absolute(pos), offset(off) {}

      private:
        const std::size_t *absolute;
        const std::size_t *offset;
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
            friend bool operator == (const Iterator &lhs, const Iterator &rhs);
            Iterator &operator ++();
            auto operator *() const -> std::pair <_Storage_t &, std::size_t> {
                return { *storage, location.get_location() };
            }
        };

        auto begin() const -> Iterator;
        auto end()   const -> Iterator;

        auto get_start() const { return begin_position; }
        void set_start(std::size_t start) { begin_position = start; }
        auto *get_local_table() const { return table; }
        auto get_offsets() const -> std::span <std::size_t> {
            return { offsets.get(), storage.size() + 1 };
        }

      private:
        friend class SymbolLocation;
        _Slice_t    storage;                    // Storage in the section
        std::size_t begin_position;             // Position in the output file
        std::unique_ptr<std::size_t[]> offsets; // Sizes of sections in the storage
        _Symbol_Table_t *table;                 // Local symbol table
    };

  private:

    static constexpr auto kSections = static_cast<std::size_t>(Section::MAXCOUNT);

    _Details_Vec_t details_vec[kSections];  // Details of the sections
    _Symbol_Table_t global_symbol_table;    // Global symbol table
    std::optional <LinkResult> result;      // Result of the linking

    void add_libc();
    void add_file(Assembler &file, _Symbol_Table_t &table);
    auto get_section(Section section) -> _Details_Vec_t &;
    void make_estimate();
    void make_relaxation();
    void link();
};

} // namespace dark
