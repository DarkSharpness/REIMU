#include "linker/linker.h"
#include "assembly/layout.h"
#include "libc/libc.h"
#include "utility/error.h"
#include <algorithm>
#include <ranges>

namespace dark {

using Iterator = Linker::StorageDetails::Iterator;

auto operator==(const Iterator &lhs, const Iterator &rhs) -> bool {
    return lhs.storage == rhs.storage;
}

auto Iterator::operator++() -> Iterator & {
    ++this->storage;
    this->location.next_offset();
    return *this;
}

Linker::StorageDetails::StorageDetails(_Slice_t storage, _Symbol_Table_t &table) :
    storage(storage), begin_position(),
    offsets(std::make_unique<target_size_t[]>(storage.size() + 1)), table(&table) {}

auto Linker::StorageDetails::begin() const -> Iterator {
    return Iterator{.storage = this->storage.data(), .location = SymbolLocation{*this, 0}};
}

auto Linker::StorageDetails::end() const -> Iterator {
    return Iterator{
        .storage  = this->storage.data() + this->storage.size(),
        .location = SymbolLocation{*this, this->storage.size()}
    };
}

Linker::SymbolLocation::SymbolLocation(const StorageDetails &details, std::size_t index) :
    absolute(&details.begin_position), offset(details.offsets.get() + index) {
    // Allow to be at the end, due to zero-size storage
    runtime_assert(index <= details.storage.size());
}

struct SymbolLocationLibc : Linker::SymbolLocation {
    explicit SymbolLocationLibc(const target_size_t &position, const target_size_t &offset) :
        Linker::SymbolLocation{&position, &offset} {}
};

/**
 * Memory Layout:
 * 1. Text Section from 0x10000 # 64KiB
 *  1.1 Libc functions
 *  1.2 User functions
 * 2. Static Data Section next
 *  2.1 Data Section
 *  2.2 RODATA Section
 *  2.3 BSS Section
 *   (all these parts may be merged into one section)
 * 3. Heap Space until 0x10000000 # 256MiB
 * 4. Stack from 0x10000000 # 256MiB
 *          to   0x20000000 # 512MiB
 */
Linker::Linker(std::span<AssemblyLayout> data) {
    std::vector<_Symbol_Table_t> local_symbol_table(data.size());
    for (std::size_t i = 0; i < data.size(); ++i)
        this->add_file(data[i], local_symbol_table[i]);

    this->add_libc();

    this->make_estimate();

    this->make_relaxation();

    this->make_estimate();

    this->link();
}

void Linker::add_file(AssemblyLayout &layout, _Symbol_Table_t &local_table) {
    using _Pair_t        = std::pair<_Storage_t *, StorageDetails *>;
    using _Section_Map_t = struct _ : std::vector<_Pair_t> {
        void add_mapping(_Storage_t *pointer, StorageDetails *details) {
            this->emplace_back(pointer, details);
        }

        auto get_location(_Storage_t *pointer) -> SymbolLocation {
            // Use projection to compare the first element only
            auto iter = std::ranges::upper_bound(*this, pointer, {}, &_Pair_t::first);

            // Find the first iter, where *iter <= pointer
            runtime_assert(--iter >= this->begin());

            // Index of the element in the section
            std::size_t index = pointer - iter->first;

            return SymbolLocation{*iter->second, index};
        }
    };

    // Initialize the section map, use flat-map
    _Section_Map_t section_map;

    for (auto &[slice, section] : layout.sections) {
        /// TODO: Fix the possible issue of labels attached some empty sections
        if (slice.empty())
            continue;
        auto &vec     = this->get_section(section);
        auto &storage = vec.emplace_back(slice, local_table);
        section_map.add_mapping(slice.data(), &storage);
    }

    std::ranges::sort(section_map, {}, &_Pair_t::first);

    // Mark the position information for the labels
    for (auto &label : layout.labels) {
        auto &[line, pointer, name, global, section] = label;

        auto location = section_map.get_location(pointer);
        auto &table   = global ? this->global_symbol_table : local_table;

        auto [iter, success] = table.try_emplace(name, location);
        panic_if(!success, "Duplicate {} symbol \"{}\"", global ? "global" : "local", name);
    }
}

/**
 * Add the libc functions to the global symbol table.
 * It will set up the starting offset of the user functions.
 */
void Linker::add_libc() {
    // An additional bias is caused by the libc functions
    static constexpr auto libc_offset = []() {
        std::array<target_size_t, std::size(libc::names)> array;
        for (auto i : std::views::iota(0llu, std::size(libc::names)))
            array[i] = i * sizeof(target_size_t);
        return array;
    }();

    std::size_t count = 0;

    for (auto &name : libc::names) {
        auto location        = SymbolLocationLibc{libc::kLibcStart, libc_offset[count++]};
        auto [iter, success] = this->global_symbol_table.try_emplace(name, location);
        panic_if(!success, "Global symbol \"{}\" conflicts with libc", name);
    }

    runtime_assert(libc::kLibcEnd == libc::kLibcStart + count * sizeof(target_size_t));
}

auto Linker::get_section(Section section) -> _Details_Vec_t & {
    auto index = static_cast<std::size_t>(section);
    runtime_assert(index < this->kSections);
    return this->details_vec[index];
}

} // namespace dark
