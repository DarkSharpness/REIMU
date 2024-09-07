// Should only be included once in src/libc/memory.cpp
#include "declarations.h"
#include "interpreter/memory.h"
#include "libc/libc.h"
#include "utility/error.h"
#include <cmath>
#include <cstddef>
#include <cstring>
#include <span>

namespace dark::libc {

struct MemoryManager {
private:
    struct Header {
    public:
        auto get_prev_size() const { return this->prev; }
        auto get_this_size() const { return this->self; }
        void set_prev_size(std::uint32_t size) { this->prev = size; }
        void set_this_size(std::uint32_t size) { this->self = size; }

    private:
        std::uint32_t prev;
        std::uint32_t self;
    };

    static constexpr target_size_t kMinAlignment = alignof(std::max_align_t);
    static constexpr target_size_t kHeaderSize   = sizeof(Header);
    static constexpr target_size_t kMinAllocSize = sizeof(void *) * 2;
    static constexpr std::size_t kMemOverhead    = 32;

    target_size_t start; // start of the heap
    target_size_t brk;   // current break, aligned to kMinAlignment

private:
    static constexpr auto align(target_size_t ptr) -> target_size_t {
        constexpr auto kMask = kMinAlignment - 1;
        return (ptr + kMask) & ~kMask;
    }

    static auto get_header(char *ptr) -> Header & {
        return *std::bit_cast<Header *>(ptr - kHeaderSize);
    }

    [[noreturn]]
    static void unknown_malloc_pointer(target_size_t, __details::_Index);

    static constexpr auto get_required_size(target_size_t size) -> target_size_t {
        return align(std::max(size + kHeaderSize, kMinAllocSize + kHeaderSize));
    }

    [[nodiscard]]
    auto allocate_required(Memory &mem, target_size_t required) {
        const auto ret_pair            = mem.sbrk(required);
        const auto [real_ptr, old_brk] = ret_pair;
        runtime_assert(this->brk == old_brk);

        this->brk += required;
        this->get_header(real_ptr).set_this_size(required);

        return ret_pair;
    }

public:
    consteval MemoryManager() : start(), brk() {}

    void init(Memory &mem) {
        // We have no restrictions on the start address
        this->start = mem.sbrk(0).second;

        // We should make sure the (brk + kHeaderSize) is aligned to kMinAlignment
        this->brk = this->align(this->start + kHeaderSize);

        auto [real_ptr, old_brk] = mem.sbrk(this->brk - this->start);

        runtime_assert(
            this->start == old_brk && std::bit_cast<std::size_t>(real_ptr) % kMinAlignment == 0
        );
    }

    [[nodiscard]]
    auto allocate(Memory &mem, target_size_t new_size) {
        return this->allocate_required(mem, this->get_required_size(new_size));
    }

    void free(Memory &, target_size_t) {
        // Do nothing for now
    }

    [[nodiscard]]
    auto reallocate(Memory &mem, target_size_t old_ptr, target_size_t new_size)
        -> std::pair<target_size_t, bool> {
        const auto area = parse_malloc_ptr(mem, old_ptr);
        auto old_data   = area.data();
        auto old_size   = area.size();
        auto required   = this->get_required_size(new_size);
        if (old_size == 0) {
            unknown_malloc_pointer(old_ptr, __details::_Index::realloc);
        } else if (old_size >= required) {
            return {old_ptr, false};
        } else {
            auto [new_data, new_ptr] = this->allocate_required(mem, required);
            std::memcpy(new_data, old_data, old_size);
            this->free(mem, old_ptr);
            return {new_ptr, true};
        }
    }

    auto parse_malloc_ptr(Memory &mem, const target_size_t malloc_ptr) const -> std::span<char> {
        if (malloc_ptr % kMinAlignment != 0)
            return {};

        const auto header_ptr = malloc_ptr - kHeaderSize;
        if (header_ptr < this->start || malloc_ptr >= this->brk)
            return {};

        auto area = mem.libc_access(header_ptr);
        if (kHeaderSize > area.size())
            return {};

        auto *data_ptr = area.data() + kHeaderSize;
        auto rest_size = area.size() - kHeaderSize;
        auto &header   = this->get_header(data_ptr);
        auto this_size = header.get_this_size();

        if (this_size % kMinAlignment != 0)
            return {};

        if (this_size > rest_size)
            return {};

        return {data_ptr, this_size};
    }

    static constexpr auto get_malloc_time(const target_size_t size) -> std::size_t {
        const auto required = get_required_size(size);
        return kMemOverhead + std::size_t(std::sqrt(required)) * 8;
    }

    static constexpr auto get_free_time() -> std::size_t { return kMemOverhead; }

    static constexpr auto get_realloc_time(const target_size_t size, bool realloc) -> std::size_t {
        constexpr std::size_t kReallocTime = 16;
        if (realloc) {
            return kReallocTime + get_malloc_time(size) + get_free_time();
        } else {
            return kReallocTime;
        }
    }
};

} // namespace dark::libc
