#pragma once
#include <interpreter/executable.h>
#include <interpreter/memory.h>
#include <libc/libc.h>
#include <utility.h>
#include <memory>

namespace dark {

struct ICache {
    using Ref_t = std::reference_wrapper<Executable>;
    explicit ICache(Memory &);
    auto ifetch(target_size_t, Executable *) noexcept -> Ref_t;
  private:
    target_size_t length;
    std::unique_ptr <Executable[]> cached;
};

static auto make_icache_range(Memory &mem) -> target_size_t {
    auto text = mem.get_text_range();
    runtime_assert(text.start == libc::kLibcEnd);
    static_assert(libc::kLibcStart == kTextStart);
    const auto size = text.finish - libc::kLibcStart;
    runtime_assert(size % sizeof(command_size_t) == 0);
    return text.finish;
}

[[maybe_unused]]
static auto no_compile(Executable &, RegisterFile &rf, Memory &mem, Device &dev) {
    Executable exe { compile_once, {} };
    return exe(rf, mem, dev);
}

[[maybe_unused]]
static auto handle_cache_miss() -> Executable & {
    static Executable exe { no_compile, {} };
    return exe;
}

/**
 * We assume the layout as below:
 * - libc functions:
 * - normal text
 */
ICache::ICache(Memory &mem) : length(make_icache_range(mem)) {
    const auto reserved = this->length / sizeof(Executable) + 1;

    // Initialize the cache

    this->cached = std::make_unique<Executable[]>(reserved);

    // Other functions are left as compile_once
    for (std::size_t i = 0 ; i < reserved ; ++i)
        this->cached[i].set_handle(compile_once, {});

    // libc functions
    for (std::size_t i = 0 ; i < std::size(libc::funcs) ; ++i)
        this->cached[i].set_handle(libc::funcs[i], {});
}

/* ifetch with some hint */
auto ICache::ifetch(target_size_t pc, Executable *hint) noexcept -> Ref_t {
    if (hint) return *hint; // Take the hint

    auto which = (pc - kTextStart) / sizeof(command_size_t);
    if (pc % alignof(command_size_t) != 0 || which >= this->length)
        return handle_cache_miss();

    return this->cached[which];
}

} // namespace dark
