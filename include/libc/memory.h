// Should only be included once in src/libc/memory.cpp
#include "declarations.h"
#include "interpreter/exception.h"
#include "interpreter/memory.h"
#include "libc/libc.h"
#include "utility/error.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <span>
#include <utility>

namespace dark::libc {

struct MemoryManager {
private:
    struct Header {
    public:
        auto get_prev_size() const -> target_size_t { return this->prev; }
        auto get_this_size() const -> target_size_t { return this->self & ~kAllocated; }
        auto is_allocated() const -> bool { return (this->self & kAllocated) != 0; }

        void set_prev_size(target_size_t size) { this->prev = size; }
        void set_this_size(target_size_t size, bool allocated) {
            this->self = size | (allocated ? kAllocated : 0);
        }

    private:
        static constexpr target_size_t kAllocated = 1;

        std::uint32_t prev;
        std::uint32_t self;
    };

    static constexpr target_size_t kMinAlignment = alignof(std::max_align_t);
    static constexpr target_size_t kHeaderSize   = sizeof(Header);
    static constexpr target_size_t kMinAllocSize = sizeof(void *) * 2;
    static constexpr target_size_t kMinBlockSize =
        (kHeaderSize + kMinAllocSize + kMinAlignment - 1) & ~(kMinAlignment - 1);
    static constexpr std::size_t kMemOverhead = 32;

    static_assert(std::has_single_bit(kMinAlignment));
    static_assert(kHeaderSize <= kMinAlignment);

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

    auto first_block() const -> target_size_t { return align(this->start + kHeaderSize); }

    auto header_at(Memory &mem, target_size_t ptr) const -> Header & {
        runtime_assert(this->first_block() <= ptr && ptr <= this->brk);

        auto area = mem.libc_access(ptr - kHeaderSize);
        runtime_assert(area.size() >= kHeaderSize);
        return get_header(area.data() + kHeaderSize);
    }

    auto valid_block(const Header &header, target_size_t ptr, target_size_t prev_size) const
        -> bool {
        const auto size = header.get_this_size();
        return header.get_prev_size() == prev_size && size >= kMinBlockSize &&
               size % kMinAlignment == 0 && size <= this->brk - ptr;
    }

    auto find_allocated_block(Memory &mem, target_size_t ptr) const -> target_size_t {
        if (ptr % kMinAlignment != 0 || ptr < this->first_block() || ptr >= this->brk)
            return 0;

        target_size_t prev_size = 0;
        for (auto current = this->first_block(); current < this->brk;) {
            const auto &header = this->header_at(mem, current);
            if (!this->valid_block(header, current, prev_size))
                return 0;

            const auto size = header.get_this_size();
            if (current == ptr)
                return header.is_allocated() ? size : 0;
            if (ptr < current + size)
                return 0;

            prev_size = size;
            current += size;
        }

        return 0;
    }

    [[noreturn]]
    static void unknown_malloc_pointer(target_size_t, __details::_Index);

    [[noreturn]]
    static void out_of_memory(target_size_t size) {
        throw FailToInterpret{.error = Error::OutOfMemory, .detail = {.address = {}, .size = size}};
    }

    static constexpr auto get_required_size(target_size_t size) -> target_size_t {
        constexpr auto kMaxIncrement = target_size_t(std::numeric_limits<target_ssize_t>::max());
        constexpr auto kMaxRequest   = kMaxIncrement - kHeaderSize - (kMinAlignment - 1);
        if (size > kMaxRequest)
            return 0;
        return align(std::max(size + kHeaderSize, kMinBlockSize));
    }

    auto allocate_from_block(Memory &mem, target_size_t ptr, target_size_t required)
        -> std::pair<char *, target_size_t> {
        auto &header         = this->header_at(mem, ptr);
        const auto size      = header.get_this_size();
        const auto remainder = size - required;

        if (remainder >= kMinBlockSize) {
            header.set_this_size(required, true);

            auto &remainder_header = this->header_at(mem, ptr + required);
            remainder_header.set_prev_size(required);
            remainder_header.set_this_size(remainder, false);
            this->header_at(mem, ptr + size).set_prev_size(remainder);
        } else {
            header.set_this_size(size, true);
        }

        auto area = mem.libc_access(ptr);
        runtime_assert(area.size() >= header.get_this_size() - kHeaderSize);
        return {area.data(), ptr};
    }

    [[nodiscard]]
    auto allocate_required(Memory &mem, target_size_t required)
        -> std::pair<char *, target_size_t> {
        target_size_t prev_size = 0;
        for (auto current = this->first_block(); current < this->brk;) {
            const auto &header = this->header_at(mem, current);
            runtime_assert(this->valid_block(header, current, prev_size));

            const auto size = header.get_this_size();
            if (!header.is_allocated() && size >= required)
                return this->allocate_from_block(mem, current, required);

            prev_size = size;
            current += size;
        }

        const auto [real_ptr, old_brk] = mem.sbrk(static_cast<target_ssize_t>(required));
        runtime_assert(this->brk == old_brk);

        this->brk += required;

        auto &header = get_header(real_ptr);
        runtime_assert(header.get_prev_size() == prev_size && header.get_this_size() == 0);
        header.set_this_size(required, true);

        auto &sentinel = this->header_at(mem, this->brk);
        sentinel.set_prev_size(required);
        sentinel.set_this_size(0, false);

        return {real_ptr, old_brk};
    }

public:
    consteval MemoryManager() : start(), brk() {}

    void init(Memory &mem) {
        // Reserve enough padding for the first header while keeping returned pointers aligned.
        this->start = mem.sbrk(0).second;
        this->brk   = this->first_block();

        auto [real_ptr, old_brk] = mem.sbrk(static_cast<target_ssize_t>(this->brk - this->start));
        runtime_assert(
            this->start == old_brk &&
            std::bit_cast<std::size_t>(real_ptr + (this->brk - this->start)) % kMinAlignment == 0
        );

        auto &sentinel = get_header(real_ptr + (this->brk - this->start));
        sentinel.set_prev_size(0);
        sentinel.set_this_size(0, false);
    }

    [[nodiscard]]
    auto allocate(Memory &mem, target_size_t new_size) -> std::pair<char *, target_size_t> {
        const auto required = this->get_required_size(new_size);
        if (required == 0)
            out_of_memory(new_size);
        return this->allocate_required(mem, required);
    }

    void free(Memory &mem, target_size_t malloc_ptr) {
        if (malloc_ptr == 0)
            return;

        auto block_size = this->find_allocated_block(mem, malloc_ptr);
        if (block_size == 0)
            unknown_malloc_pointer(malloc_ptr, __details::_Index::free);

        auto block_ptr = malloc_ptr;
        this->header_at(mem, block_ptr).set_this_size(block_size, false);

        auto next_prev_size = block_size;
        while (block_size < this->brk - block_ptr) {
            const auto next_ptr     = block_ptr + block_size;
            const auto &next_header = this->header_at(mem, next_ptr);
            runtime_assert(this->valid_block(next_header, next_ptr, next_prev_size));
            if (next_header.is_allocated())
                break;
            next_prev_size = next_header.get_this_size();
            block_size += next_prev_size;
        }

        while (const auto prev_size = this->header_at(mem, block_ptr).get_prev_size()) {
            runtime_assert(prev_size <= block_ptr - this->first_block());
            const auto prev_ptr     = block_ptr - prev_size;
            const auto &prev_header = this->header_at(mem, prev_ptr);
            runtime_assert(prev_header.get_this_size() == prev_size);
            if (prev_header.is_allocated())
                break;

            block_ptr = prev_ptr;
            block_size += prev_size;
        }

        auto &header = this->header_at(mem, block_ptr);
        header.set_this_size(block_size, false);
        this->header_at(mem, block_ptr + block_size).set_prev_size(block_size);

        if (block_ptr + block_size != this->brk)
            return;

        const auto prev_size    = header.get_prev_size();
        const auto [_, old_brk] = mem.sbrk(-static_cast<target_ssize_t>(block_size));
        runtime_assert(old_brk == this->brk);
        this->brk = block_ptr;

        auto &sentinel = this->header_at(mem, this->brk);
        sentinel.set_prev_size(prev_size);
        sentinel.set_this_size(0, false);
    }

    [[nodiscard]]
    auto reallocate(Memory &mem, target_size_t old_ptr, target_size_t new_size)
        -> std::pair<target_size_t, bool> {
        if (old_ptr == 0) {
            auto [_, new_ptr] = this->allocate(mem, new_size);
            return {new_ptr, true};
        }

        const auto old_area = this->parse_malloc_ptr(mem, old_ptr);
        if (old_area.empty())
            unknown_malloc_pointer(old_ptr, __details::_Index::realloc);
        const auto old_size = old_area.size();
        if (old_size >= new_size)
            return {old_ptr, false};

        auto [new_data, new_ptr]    = this->allocate(mem, new_size);
        const auto current_old_area = this->parse_malloc_ptr(mem, old_ptr);
        runtime_assert(current_old_area.size() == old_size);
        std::memcpy(new_data, current_old_area.data(), old_size);
        this->free(mem, old_ptr);
        return {new_ptr, true};
    }

    auto parse_malloc_ptr(Memory &mem, target_size_t malloc_ptr) const -> std::span<char> {
        const auto block_size = this->find_allocated_block(mem, malloc_ptr);
        if (block_size == 0)
            return {};

        const auto content_size = block_size - kHeaderSize;
        auto area               = mem.libc_access(malloc_ptr);
        if (content_size > area.size())
            return {};
        return {area.data(), content_size};
    }

    static constexpr auto get_malloc_time(target_size_t size) -> std::size_t {
        const auto required = get_required_size(size);
        return kMemOverhead + std::size_t(std::sqrt(required)) * 8;
    }

    static constexpr auto get_free_time() -> std::size_t { return kMemOverhead; }

    static constexpr auto get_realloc_time(target_size_t size, bool realloc) -> std::size_t {
        constexpr std::size_t kReallocTime = 16;
        if (realloc) {
            return kReallocTime + get_malloc_time(size) + get_free_time();
        } else {
            return kReallocTime;
        }
    }
};

} // namespace dark::libc
