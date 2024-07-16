// Should only be included once in src/libc/memory.cpp
#include <libc/libc.h>
#include <utility.h>
#include <interpreter/memory.h>

namespace dark::libc {

static struct MemoryManager {
    struct Header {
        std::uint32_t prev;
        std::uint32_t self;
    };

    static constexpr target_size_t kMinAlignment = alignof(std::max_align_t);
    static constexpr target_size_t kHeaderSize   = sizeof(Header);
    static constexpr target_size_t kMinAllocSize = sizeof(void *) * 2;

    target_size_t start;    // start of the heap
    target_size_t brk;      // current break, aligned to kMinAlignment

    consteval MemoryManager() : start(), brk() {}

    void init(Memory &mem) {
        constexpr auto kMask = kMinAlignment - 1;

        // We have no restrictions on the start address
        this->start = mem.sbrk(0).second;

        // We should make sure the (brk + kHeaderSize) is aligned to kMinAlignment
        this->brk = (this->start + kHeaderSize + kMask) & ~kMask;

        auto [real_ptr, old_brk] = mem.sbrk(this->brk - this->start);

        runtime_assert(
            this->start == old_brk &&
            std::bit_cast <std::size_t> (real_ptr) % kMinAlignment == 0);
    }

    static auto get_header(char *ptr) -> Header & {
        return *std::bit_cast <Header *> (ptr - kHeaderSize);
    }

    [[nodiscard]]
    auto allocate(Memory &mem, target_size_t required) {
        constexpr auto kMask = kMinAlignment - 1;

        required = std::max(required, kMinAllocSize);
        required = (required + kHeaderSize + kMask) & ~kMask;

        const auto ret_pair = mem.sbrk(required);
        const auto [real_ptr, old_brk] = ret_pair;
        runtime_assert(this->brk == old_brk);

        this->brk += required;
        this->get_header(real_ptr).self = required;

        return ret_pair;
    }

    void free(Memory &, target_size_t) {
        // Do nothing for now
    }

    auto malloc_usable_size(char *ptr) -> target_size_t {
        return get_header(ptr).self - kHeaderSize;
    }
} malloc_manager;

void libc_init(RegisterFile &, Memory &mem, Device &) {
    malloc_manager.init(mem);
}

} // namespace dark::libc
