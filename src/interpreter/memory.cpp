#include <interpreter/memory.h>
#include <utility.h>
#include <cstddef>

namespace dark {

struct Memory::Impl : Memory {
    target_size_t text_start;
    target_size_t text_finish;
    target_size_t heap_start;

    alignas(std::size_t) std::byte storage[];
};

auto Memory::get_impl() -> Impl & {
    return static_cast <Impl &> (*this);
}




} // namespace dark
