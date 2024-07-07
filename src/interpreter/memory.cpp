#include <interpreter/area.h>
#include <interpreter/memory.h>
#include <interpreter/exception.h>
#include <interpreter/executable.h>
#include <libc/libc.h>
#include <utility.h>
#include <cstddef>
#include <array>
#include <ranges>
#include <algorithm>

namespace dark {

using i8    = std::int8_t;
using i16   = std::int16_t;
using i32   = std::int32_t;
using u8    = std::uint8_t;
using u16   = std::uint16_t;
using u32   = std::uint32_t;

template <std::integral _Int>
static _Int &int_cast(std::byte *ptr) {
    return *reinterpret_cast <_Int *> (ptr);
}

/**
 * Real Memory layout:
 * - Libc text
 * - Text
 * - Data | RoData | Bss | Heap
 */
struct Memory_Impl : StaticArea, HeapArea, StackArea {
    explicit Memory_Impl(const Config &config, const MemoryLayout &layout)
        : StaticArea(layout), HeapArea(layout), StackArea(config) {}

    auto checked_ifetch(target_size_t) -> target_size_t;

    template <std::integral _Int>
    auto checked_load(target_size_t) -> _Int;
 
    template <std::unsigned_integral _Int>
    void check_store(target_size_t, _Int);

    auto fetch_exe(target_size_t) -> Executable &;
    auto fetch_exe_libc(target_size_t) -> Executable &;
};

struct Memory::Impl : Memory, Memory_Impl {
    explicit Impl(const Config &config, const MemoryLayout &layout) :
        Memory(), Memory_Impl(config, layout) {}
};

auto Memory::get_impl() -> Impl & {
    return static_cast <Impl &> (*this);
}

auto Memory::load_i8(target_size_t addr) -> std::int8_t {
    return this->get_impl().checked_load <i8> (addr);
}

auto Memory::load_i16(target_size_t addr) -> std::int16_t {
    return this->get_impl().checked_load <i16> (addr);
}

auto Memory::load_i32(target_size_t addr) -> std::int32_t {
    return this->get_impl().checked_load <i32> (addr);
}

auto Memory::load_u8(target_size_t addr) -> std::uint8_t {
    return this->get_impl().checked_load <u8> (addr);
}

auto Memory::load_u16(target_size_t addr) -> std::uint16_t {
    return this->get_impl().checked_load <u16> (addr);
}

auto Memory::load_u32(target_size_t addr) -> u32 {
    return this->get_impl().checked_load <u32> (addr);
}

auto Memory::load_cmd(target_size_t addr) -> u32 {
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

auto Memory::create(const Config &config, const MemoryLayout &result)
-> std::unique_ptr <Memory> {
    auto ptr = new Memory::Impl { config, result };
    auto ret = std::unique_ptr <Memory> { ptr };

    auto [static_low, static_top] = static_cast <StaticArea *> (ptr)->get_range();
    auto [heap_low, heap_top] = static_cast <HeapArea *> (ptr)->get_range();
    auto [stack_low, stack_top] = static_cast <StackArea *> (ptr)->get_range();

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
            kTextStart,
            static_low, static_top, static_top - static_low,
            stack_low,  stack_top,  stack_top  - stack_low,
            stack_top,
            heap_top + stack_top - stack_low);



    return ret;
}

Memory::~Memory() {
    static_cast <Memory_Impl &> (this->get_impl()).~Memory_Impl();
}

auto Memory::fetch_executable(target_size_t pc) -> Executable & {
    return this->get_impl().fetch_exe(pc); 
}

auto Memory_Impl::checked_ifetch(target_size_t pc) -> target_size_t {
    if (pc % alignof(pc) != 0)
        throw FailToInterpret { Error::InsMisAligned, pc, alignof(pc) };

    if (!this->in_text(pc, pc + sizeof(pc)))
        throw FailToInterpret { Error::InsOutOfBound, pc };

    return int_cast <target_size_t> (this->get_static(pc));
}

template <std::integral _Int>
auto Memory_Impl::checked_load(target_size_t addr) -> _Int {
    if (addr % alignof(_Int) != 0)
        throw FailToInterpret { Error::LoadMisAligned, addr, alignof(_Int) };

    if (this->in_data(addr, addr + sizeof(_Int)))
        return int_cast <_Int> (this->get_static(addr));

    if (this->in_heap(addr, addr + sizeof(_Int)))
        return int_cast <_Int> (this->get_heap(addr));

    if (this->in_stack(addr, addr + sizeof(_Int)))
        return int_cast <_Int> (this->get_stack(addr));

    throw FailToInterpret { Error::LoadOutOfBound, addr };
}

template <std::unsigned_integral _Int>
void Memory_Impl::check_store(target_size_t addr, _Int val) {
    if (addr % alignof(_Int) != 0)
        throw FailToInterpret { Error::StoreMisAligned, addr, alignof(_Int) };

    if (this->in_data(addr, addr + sizeof(_Int)))
        return void(int_cast <_Int> (this->get_static(addr)) = val);

    if (this->in_heap(addr, addr + sizeof(_Int)))
        return void(int_cast <_Int> (this->get_heap(addr)) = val);

    if (this->in_stack(addr, addr + sizeof(_Int)))
        return void(int_cast <_Int> (this->get_stack(addr)) = val);

    throw FailToInterpret { Error::StoreOutOfBound, addr };
}

/**
 * 2 cases:
 * - 1. Builtin libc functions, return a user-written handle.
 * - 2. User-defined functions, return the actual function handle.
 */
auto Memory_Impl::fetch_exe(target_size_t pc) -> Executable & {
    if (pc < libc::kLibcEnd)
        return this->fetch_exe_libc(pc);

    this->checked_ifetch(pc);
    return this->unchecked_fetch_exe(pc);
}

auto Memory_Impl::fetch_exe_libc(target_size_t pc) -> Executable & {
    if (pc % alignof(pc) != 0)
        throw FailToInterpret { Error::InsMisAligned, pc, alignof(pc) };

    if (pc < libc::kLibcStart)
        throw FailToInterpret { Error::InsOutOfBound, pc, alignof(pc) };

    static auto libc_exe = []() {
        std::array <Executable, std::size(libc::funcs)> result;
        std::size_t i = 0;
        for (auto *fn : libc::funcs) result.at(i++).set_handle(fn, {});
        runtime_assert(i == std::size(libc::funcs));
        return result;
    }();

    return libc_exe.at((pc - libc::kLibcStart) / sizeof(pc));
}

} // namespace dark
