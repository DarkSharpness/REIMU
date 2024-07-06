// Should only be included in memory.cpp
#include <declarations.h>
#include <utility/config.h>
#include <interpreter/executable.h>
#include <linker/layout.h>
#include <memory>
#include <vector>
#include <ranges>
#include <algorithm>

namespace dark {

static auto align_to_page(target_size_t addr) {
    return (addr + 0xfff) & ~0xfff;
}

struct StaticArea {
  private:
    target_size_t const text_start;
    target_size_t const text_finish;
    target_size_t const data_start;
    target_size_t const data_finish;
    std::byte *const storage;

    static constexpr auto kTextSize = sizeof(target_size_t);

  public:
    std::vector <Executable> exes;

    explicit StaticArea(const MemoryLayout &layout) :
        text_start(layout.text.begin()),
        text_finish(layout.text.end()),
        data_start(layout.data.begin()),
        data_finish(layout.bss.end()),
        storage(new std::byte[data_finish - text_start] {} - text_start),
        exes((text_finish - text_start) / kTextSize)
    {
        constexpr auto __copy = [](StaticArea *area, const auto &section) {
            std::ranges::copy(section.storage, area->get_static(section.begin()));
        };
        __copy(this, layout.text);
        __copy(this, layout.data);
        __copy(this, layout.rodata);
        __copy(this, layout.bss);
    }
    bool in_text(target_size_t lo, target_size_t hi) const {
        return this->text_start <= lo && hi <= this->text_finish;
    }
    bool in_data(target_size_t lo, target_size_t hi) const {
        return this->data_start <= lo && hi <= this->data_finish;
    }
    auto get_static(target_size_t addr) -> std::byte * {
        return this->storage + addr;
    }
    auto unchecked_fetch_exe(target_size_t pc) -> Executable & {
        return this->exes[(pc - this->text_start) / kTextSize];
    }
    ~StaticArea() { delete (this->storage + this->text_start); }
};

struct HeapArea {
  private:
    std::vector <std::byte> storage;
    target_size_t const heap_start;
    target_size_t       heap_finish;
  public:
    explicit HeapArea(const MemoryLayout &layout) :
        heap_start(align_to_page(layout.bss.end())),
        heap_finish(heap_start) {}
    bool in_heap(target_size_t lo, target_size_t hi) {
        return this->heap_start <= lo && hi <= this->heap_finish;
    }
    auto *get_heap(target_size_t addr) {
        return this->storage.data() - this->heap_start + addr;
    }
};

struct StackArea {
  private:
    target_size_t const stack_start;
    target_size_t const stack_finish;
    std::byte * const storage;
  public:
    explicit StackArea(const Config &config) :
        stack_start(config.storage_size - config.stack_size),
        stack_finish(config.storage_size),
        storage(new std::byte[stack_finish - stack_start] {} - stack_start)
    {}

    bool in_stack(target_size_t lo, target_size_t hi) const {
        return this->stack_start <= lo && hi <= this->stack_finish;
    }
    auto *get_stack(target_size_t addr) {
        return this->storage + addr;
    }
    ~StackArea() { delete (this->storage + this->stack_start); }
};

} // namespace dark
