#include <interpreter/memory.h>
#include <interpreter/exception.h>
#include <interpreter/executable.h>
#include <linker/memory.h>
#include <libc/libc.h>
#include <utility.h>
#include <utility/config.h>
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

struct Memory_Impl_Data {
    target_size_t text_start;
    target_size_t text_finish;
    target_size_t mem_start;
    target_size_t mem_finish;
    target_size_t heap_start;
    std::vector <Executable> exes;
};

struct Memory::Impl : Memory, Memory_Impl_Data {
    std::byte *get_storage() { return reinterpret_cast <std::byte *> (this); }

    template <std::integral _Int>
    auto checked_access(target_size_t addr) -> _Int {
        if (addr < this->mem_start || addr > this->mem_finish - sizeof(_Int))
            throw FailToInterpret { Error::LoadOutOfBound, addr, sizeof(_Int) };

        if (addr % sizeof(_Int) != 0)
            throw FailToInterpret { Error::LoadMisAligned, addr, sizeof(_Int) };

        return *reinterpret_cast <_Int *> (this->get_storage() + addr);
    }

    auto checked_access_cmd(target_size_t addr) -> target_size_t {
        static_assert(sizeof(target_size_t) == 4);

        if (addr < this->text_start || addr > this->text_finish - 4)
            throw FailToInterpret { Error::InsOutOfBound, addr, 4 };

        if (addr % 4 != 0)
            throw FailToInterpret { Error::InsMisAligned, addr, 4 };

        return *reinterpret_cast <target_size_t *> (this->get_storage() + addr);
    }

    template <std::integral _Int>
    void checked_write(target_size_t addr, _Int value) {
        if (addr < this->mem_start || addr > this->mem_finish - sizeof(_Int))
            throw FailToInterpret { Error::StoreOutOfBound, addr, sizeof(_Int) };

        if (addr % sizeof(_Int) != 0)
            throw FailToInterpret { Error::StoreMisAligned, addr, sizeof(_Int) };

        *reinterpret_cast <u32 *> (this->get_storage() + addr) = value;
    }

    auto fetch_exe(target_size_t) ->Executable &;
    auto fetch_exe_libc(target_size_t) -> Executable &;
};

static_assert(sizeof(Memory::Impl) <= libc::kLibcStart,
    "Memory::Impl is too large, which overlaps with the real program");

auto Memory::get_impl() -> Impl & {
    return static_cast <Impl &> (*this);
}

auto Memory::load_i8(target_size_t addr) -> std::int8_t {
    return this->get_impl().checked_access <i8> (addr);
}

auto Memory::load_i16(target_size_t addr) -> std::int16_t {
    return this->get_impl().checked_access <i16> (addr);
}

auto Memory::load_i32(target_size_t addr) -> std::int32_t {
    return this->get_impl().checked_access <i32> (addr);
}

auto Memory::load_u8(target_size_t addr) -> std::uint8_t {
    return this->get_impl().checked_access <u8> (addr);
}

auto Memory::load_u16(target_size_t addr) -> std::uint16_t {
    return this->get_impl().checked_access <u16> (addr);
}

auto Memory::load_u32(target_size_t addr) -> u32 {
    return this->get_impl().checked_access <u32> (addr);
}

auto Memory::load_cmd(target_size_t addr) -> u32 {
    return this->get_impl().checked_access_cmd(addr);
}

void Memory::store_u8(target_size_t addr, u8 value) {
    this->get_impl().checked_write(addr, value);
}

void Memory::store_u16(target_size_t addr, u16 value) {
    this->get_impl().checked_write(addr, value);
}

void Memory::store_u32(target_size_t addr, u32 value) {
    this->get_impl().checked_write(addr, value);
}

auto Memory::create(const Config &config, const MemoryLayout &result) -> std::unique_ptr <Memory> {
    // Extra sizeof(target_size_t) zero bytes for safe string operations
    const auto length = result.bss.end() + sizeof(target_size_t);
    auto *mem = new std::byte[length];
    std::ranges::fill_n(mem, length, std::byte{});

    auto *ptr = std::launder(reinterpret_cast <Memory::Impl *> (mem));
    new (ptr) Memory::Impl;

    // Initialize the memory layout
    runtime_assert(result.text.start == libc::kLibcEnd);
    // The memory must be at least aligned to the size of target_size_t
    runtime_assert(reinterpret_cast <std::size_t> (mem) % alignof(target_size_t) == 0);

    ptr->text_start = result.text.begin();
    ptr->text_finish = result.text.end();

    static_assert(sizeof(std::byte) == sizeof(char));
    std::ranges::copy(result.text.storage, mem + result.text.begin());

    ptr->mem_start = result.data.begin();
    ptr->mem_finish = config.storage_size;
    ptr->heap_start = result.bss.end();

    std::ranges::copy(result.data.storage, mem + result.data.begin());
    std::ranges::copy(result.rodata.storage, mem + result.rodata.begin());
    std::ranges::copy(result.bss.storage, mem + result.bss.begin());

    ptr->exes.resize((ptr->text_finish - ptr->text_start) / sizeof(target_size_t));

    return std::unique_ptr <Memory> { ptr };
}

Memory::~Memory() {
    static_cast <Memory_Impl_Data &> (this->get_impl()).~Memory_Impl_Data();
}

auto Memory::fetch_executable(target_size_t pc) -> Executable & {
    return this->get_impl().fetch_exe(pc); 
}

auto Memory::Impl::fetch_exe(target_size_t pc) -> Executable & {
    // 2 cases:
    // - 1. builtin libc functions, return a dummy handle.
    // - 2. user-defined functions, return the actual function handle.
    if (pc >= libc::kLibcStart && pc <= libc::kLibcEnd - sizeof(target_size_t))
        return this->fetch_exe_libc(pc);

    this->checked_access_cmd(pc);

    auto which = (pc - this->text_start) / sizeof(target_size_t);
    return this->exes.at(which);
}

auto Memory::Impl::fetch_exe_libc(target_size_t pc) -> Executable & {
    using _Array_t = std::array <Executable, std::size(libc::funcs)>;
    static auto libc_exe = []() {
        _Array_t result;
        std::size_t i = 0;
        for (auto *fn : libc::funcs)
            result.at(i++).set_handle(fn, 0);
        return result;
    }();

    if (pc % sizeof(target_size_t) != 0)
        throw FailToInterpret { Error::InsMisAligned, pc, sizeof(target_size_t) };

    auto which = (pc - libc::kLibcStart) / sizeof(target_size_t);
    return libc_exe.at(which);
}

} // namespace dark
