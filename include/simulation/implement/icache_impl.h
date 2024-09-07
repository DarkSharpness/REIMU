#include "declarations.h"
#include "interpreter/interval.h"
#include "libc/libc.h"
#include "simulation/implement/icache_decl.h"
#include "utility/error.h"

namespace dark {

// This function is implemented in interpreter/executable.cpp
Function_t compile_once;

static auto make_icache_range(Memory &mem) -> target_size_t {
    const auto text = mem.get_text_range();
    runtime_assert(text.start == libc::kLibcEnd);
    static_assert(libc::kLibcStart == kTextStart);
    const auto size = text.finish - libc::kLibcStart;
    runtime_assert(size % sizeof(command_size_t) == 0);
    return size / sizeof(command_size_t);
}

[[maybe_unused]]
static auto compile_always(Executable &, RegisterFile &rf, Memory &mem, Device &dev) {
    Executable exe { compile_once, {} };
    return exe(rf, mem, dev);
}

[[maybe_unused]]
static auto handle_cache_miss() -> Executable & {
    static Executable exe { compile_always, {} };
    return exe;
}

/**
 * We assume the layout as below:
 * - libc functions:
 * - normal text
 */
inline ICache::ICache(Memory &mem) : length(make_icache_range(mem)) {
    const auto reserved = this->length;
    const auto libcsize = std::size(libc::funcs);

    // Initialize the cache
    this->cached = std::make_unique<Executable[]>(reserved);

    // libc functions
    for (std::size_t i = 0 ; i < libcsize ; ++i)
        this->cached[i].set_handle(libc::funcs[i], {});

    // Other functions are left as compile_once
    for (std::size_t i = libcsize ; i < reserved ; ++i)
        this->cached[i].set_handle(compile_once, {});
}

/* ifetch with some hint */
inline auto ICache::ifetch(target_size_t pc, Hint hint) noexcept -> Executable & {
    if (std::size_t(hint.next - this->cached.get()) < this->length)
        [[likely]] return *hint.next;

    std::size_t which = (pc - kTextStart) / sizeof(command_size_t);
    if (pc % alignof(command_size_t) != 0 || which >= this->length)
        return handle_cache_miss();

    return this->cached[which];
}

} // namespace dark
