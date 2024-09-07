#include "interpreter/memory.h"
#include "declarations.h"
#include "interpreter/exception.h"
#include "simulation/area.h"
#include "utility/misc.h"
#include <cstddef>
#include <memory>

namespace dark {

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;

template <std::integral _Int>
static auto int_cast(std::byte *ptr) -> _Int & {
    return *reinterpret_cast<_Int *>(ptr);
}

/**
 * Real Memory layout:
 * - Libc text
 * - Text
 * - Data | RoData | Bss | Heap
 */
struct Memory_Impl : StaticArea, HeapArea, StackArea {
    explicit Memory_Impl(const Config &config, const MemoryLayout &layout) :
        StaticArea(layout), HeapArea(layout), StackArea(config) {}

    auto checked_ifetch(target_size_t) -> command_size_t;

    template <std::integral _Int>
    auto checked_load(target_size_t) -> _Int;

    template <std::unsigned_integral _Int>
    void check_store(target_size_t, _Int);

    auto get_segment(target_size_t) -> std::span<char>;
};

struct Memory::Impl : Memory, Memory_Impl {
    explicit Impl(const Config &config, const MemoryLayout &layout) :
        Memory(), Memory_Impl(config, layout) {}
};

auto Memory::get_impl() -> Impl & {
    return static_cast<Impl &>(*this);
}

auto Memory::load_i8(target_size_t addr) -> i8 {
    return this->get_impl().checked_load<i8>(addr);
}

auto Memory::load_i16(target_size_t addr) -> i16 {
    return this->get_impl().checked_load<i16>(addr);
}

auto Memory::load_i32(target_size_t addr) -> i32 {
    return this->get_impl().checked_load<i32>(addr);
}

auto Memory::load_u8(target_size_t addr) -> u8 {
    return this->get_impl().checked_load<u8>(addr);
}

auto Memory::load_u16(target_size_t addr) -> u16 {
    return this->get_impl().checked_load<u16>(addr);
}

auto Memory::load_u32(target_size_t addr) -> u32 {
    return this->get_impl().checked_load<u32>(addr);
}

auto Memory::load_cmd(target_size_t addr) -> command_size_t {
    return this->get_impl().checked_ifetch(addr);
}

void Memory::store_u8(target_size_t addr, u8 value) {
    this->get_impl().check_store(addr, value);
}

void Memory::store_u16(target_size_t addr, u16 value) {
    this->get_impl().check_store(addr, value);
}

void Memory::store_u32(target_size_t addr, u32 value) {
    this->get_impl().check_store(addr, value);
}

auto Memory::create(const Config &config, const MemoryLayout &result) -> unique_t {
    auto retval = std::make_unique<Impl>(config, result);
    auto *ptr   = retval.get();

    auto [static_low, static_top] = static_cast<StaticArea *>(ptr)->get_range();
    auto [heap_low, heap_top]     = static_cast<HeapArea *>(ptr)->get_range();
    auto [stack_low, stack_top]   = static_cast<StackArea *>(ptr)->get_range();

    if (static_top > heap_low || heap_top > stack_low)
        panic(
            "Not enough memory for the program!\n"
            "  Hint: In RISC-V, the lowest {0} bytes are reserved.\n"
            "        Text section starts from 0x{0:x}.\n"
            "        Note that sections are aligned to 4096 bytes,\n"
            "          and there's some space reserved for libc functions,\n"
            "          which means that some extra memory might be needed.\n"
            "        Current program:\t[0x{1:x}, 0x{2:x}),\tsize = {3}\n"
            "        Current stack:  \t[0x{4:x}, 0x{5:x}),\tsize = {6}\n"
            "        Current memory size: {7}\n"
            "        Minimum memory size: {8}\n",
            kTextStart, static_low, static_top, static_top - static_low, stack_low, stack_top,
            stack_top - stack_low, stack_top, heap_top + stack_top - stack_low
        );

    return retval;
}

auto Memory::sbrk(target_ssize_t inc) -> std::pair<char *, target_size_t> {
    return this->get_impl().grow(inc);
}

auto Memory::get_text_range() -> Interval {
    return static_cast<StaticArea &>(this->get_impl()).get_text_range();
}

auto Memory::libc_access(target_size_t addr) -> std::span<char> {
    return this->get_impl().get_segment(addr);
}

void Memory::print_details(bool detail) const {
    allow_unused(detail);
}

auto Memory::get_heap_start() const -> target_size_t {
    return static_cast<const HeapArea *>(static_cast<const Impl *>(this))->get_range().start;
}

auto Memory::get_stack_start() const -> target_size_t {
    return static_cast<const StackArea *>(static_cast<const Impl *>(this))->get_range().start;
}

auto Memory::get_stack_end() const -> target_size_t {
    return static_cast<const StackArea *>(static_cast<const Impl *>(this))->get_range().finish;
}

template <Error error, std::integral _Int>
[[noreturn]]
static void handle_misaligned(target_size_t addr) {
    throw FailToInterpret{.error = error, .detail = {.address = addr, .size = sizeof(_Int)}};
}

template <Error error, std::integral _Int>
[[noreturn]]
static void handle_outofbound(target_size_t addr) {
    throw FailToInterpret{.error = error, .detail = {.address = addr, .size = sizeof(_Int)}};
}

auto Memory_Impl::checked_ifetch(target_size_t pc) -> command_size_t {
    if (pc % alignof(command_size_t) != 0)
        handle_misaligned<Error::InsMisAligned, command_size_t>(pc);

    if (!this->in_text(pc))
        handle_outofbound<Error::InsOutOfBound, command_size_t>(pc);

    return int_cast<target_size_t>(this->get_static(pc));
}

template <std::integral _Int>
auto Memory_Impl::checked_load(target_size_t addr) -> _Int {
    if (addr % alignof(_Int) != 0)
        handle_misaligned<Error::LoadMisAligned, _Int>(addr);

    if (addr >= target_size_t(-sizeof(_Int)))
        handle_outofbound<Error::LoadOutOfBound, _Int>(addr);

    if (this->in_data(addr, addr + sizeof(_Int)))
        return int_cast<_Int>(this->get_static(addr));

    if (this->in_heap(addr, addr + sizeof(_Int)))
        return int_cast<_Int>(this->get_heap(addr));

    if (this->in_stack(addr, addr + sizeof(_Int)))
        return int_cast<_Int>(this->get_stack(addr));

    handle_outofbound<Error::LoadOutOfBound, _Int>(addr);
}

template <std::unsigned_integral _Int>
void Memory_Impl::check_store(target_size_t addr, _Int val) {
    if (addr % alignof(_Int) != 0)
        handle_misaligned<Error::StoreMisAligned, _Int>(addr);

    if (addr >= -sizeof(_Int))
        handle_outofbound<Error::LoadOutOfBound, _Int>(addr);

    if (this->in_data(addr, addr + sizeof(_Int)))
        return void(int_cast<_Int>(this->get_static(addr)) = val);

    if (this->in_heap(addr, addr + sizeof(_Int)))
        return void(int_cast<_Int>(this->get_heap(addr)) = val);

    if (this->in_stack(addr, addr + sizeof(_Int)))
        return void(int_cast<_Int>(this->get_stack(addr)) = val);

    handle_outofbound<Error::StoreOutOfBound, _Int>(addr);
}

auto Memory_Impl::get_segment(target_size_t addr) -> std::span<char> {
    constexpr auto __wash = [](std::byte *ptr) -> char * {
        /// TODO: May be we should use std::launder here
        // to avoid potential undefined behavior,
        // but it's not necessary for now (hope so).
        return std::bit_cast<char *>(ptr);
    };

    if (auto [low, top] = StaticArea::get_data_range(); low <= addr && addr <= top)
        return {__wash(this->get_static(addr)), top - addr};

    if (auto [low, top] = HeapArea::get_range(); low <= addr && addr <= top)
        return {__wash(this->get_heap(addr)), top - addr};

    if (auto [low, top] = StackArea::get_range(); low <= addr && addr <= top)
        return {__wash(this->get_stack(addr)), top - addr};

    return {}; // Not found.
}

} // namespace dark
