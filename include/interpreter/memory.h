#pragma once
#include "declarations.h"
#include "interpreter/forward.h"
#include "utility/deleter.h"
#include <span>

namespace dark {

struct MemoryLayout;

struct Memory {
private:
    using unique_t = dark::derival_ptr<Memory>;

public:
    static auto create(const Config &, const MemoryLayout &) -> unique_t;

    auto load_i8(target_size_t addr) -> std::int8_t;
    auto load_i16(target_size_t addr) -> std::int16_t;
    auto load_i32(target_size_t addr) -> std::int32_t;
    auto load_u8(target_size_t addr) -> std::uint8_t;
    auto load_u16(target_size_t addr) -> std::uint16_t;
    [[deprecated]]
    auto load_u32(target_size_t addr) -> std::uint32_t;

    auto load_cmd(target_size_t addr) -> command_size_t;

    void store_u8(target_size_t addr, std::uint8_t value);
    void store_u16(target_size_t addr, std::uint16_t value);
    void store_u32(target_size_t addr, std::uint32_t value);

    // For malloc use.
    [[nodiscard]]
    auto sbrk(target_ssize_t) -> std::pair<char *, target_size_t>;

    // For libc functions.
    auto libc_access(target_size_t) -> std::span<char>;

    // For ICache.
    auto get_text_range() -> Interval;

    void print_details(bool) const;

    auto get_heap_start() const -> target_size_t;
    auto get_stack_start() const -> target_size_t;
    auto get_stack_end() const -> target_size_t;

private:
    struct Impl;
    Impl &get_impl();
};

} // namespace dark
