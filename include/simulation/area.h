// Should only be included in interpreter/memory.cpp
#include "config/config.h"
#include "interpreter/forward.h"
#include "interpreter/interval.h"
#include "libc/libc.h"
#include "linker/layout.h"
#include "utility/error.h"
#include <algorithm>
#include <vector>

namespace dark {

namespace {

struct StaticArea {
private:
    const Interval text;
    const Interval data;
    std::byte *const storage;

public:
    explicit StaticArea(const MemoryLayout &layout) :
        text({layout.text.begin(), layout.text.end()}),
        data({layout.data.begin(), layout.bss.end()}),
        storage(new std::byte[data.finish - text.start]{} - text.start) {
        runtime_assert(text.start == libc::kLibcEnd);
        constexpr auto __copy = [](StaticArea *area, const auto &section) {
            std::ranges::copy(section.storage, area->get_static(section.begin()));
        };
        __copy(this, layout.text);
        __copy(this, layout.data);
        __copy(this, layout.rodata);
        __copy(this, layout.bss);
    }
    bool in_text(target_size_t pc) const {
        return this->text.start <= pc && pc < this->text.finish;
    }
    bool in_data(target_size_t lo, target_size_t hi) const { return this->data.contains(lo, hi); }
    auto get_static(target_size_t addr) -> std::byte * { return this->storage + addr; }
    auto get_range() const -> Interval { return {this->text.start, this->data.finish}; }
    auto get_text_range() const { return this->text; }
    auto get_data_range() const { return this->data; }
    ~StaticArea() { delete (this->storage + this->text.start); }
};

struct HeapArea {
private:
    std::vector<std::byte> storage;
    const target_size_t heap_start;
    target_size_t heap_finish;
    // Our implementation require that the end of static area
    // should not overlap with the start of heap area
    // So, we need to choose the next page even if already aligned
    static auto next_page(target_size_t addr) -> target_size_t {
        constexpr target_size_t kPageSize = 1 << 12;
        return (addr & ~(kPageSize - 1)) + kPageSize;
    }

public:
    explicit HeapArea(const MemoryLayout &layout) :
        heap_start(next_page(layout.bss.end())), heap_finish(heap_start) {}
    bool in_heap(target_size_t lo, target_size_t hi) const {
        return this->heap_start <= lo && hi <= this->heap_finish;
    }
    auto *get_heap(target_size_t addr) { return this->storage.data() - this->heap_start + addr; }
    auto get_range() const -> Interval { return {this->heap_start, this->heap_finish}; }
    auto grow(target_ssize_t size) {
        /// TODO: remove this useless check
        const auto old_size = this->heap_finish - this->heap_start;
        runtime_assert(old_size == this->storage.size());

        const auto retval = this->heap_finish;
        this->heap_finish += size;
        const auto new_size = old_size + size;

        // To avoid too many reallocations, we reserve the next power of 2
        if (size > 0)
            this->storage.reserve(std::bit_ceil(new_size));
        this->storage.resize(new_size);

        return std::make_pair(std::bit_cast<char *>(this->get_heap(retval)), retval);
    }
};

struct StackArea {
private:
    const Interval stack;
    std::byte *const storage;

public:
    explicit StackArea(const Config &config) :
        stack({config.get_stack_low(), config.get_stack_top()}),
        storage(new std::byte[stack.finish - stack.start]{} - stack.start) {}

    bool in_stack(target_size_t lo, target_size_t hi) const { return this->stack.contains(lo, hi); }
    auto *get_stack(target_size_t addr) { return this->storage + addr; }
    auto get_range() const { return this->stack; }
    ~StackArea() { delete (this->storage + this->stack.start); }
};

} // namespace

} // namespace dark
