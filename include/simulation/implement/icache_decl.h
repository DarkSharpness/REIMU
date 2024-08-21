#include <interpreter/executable.h>
#include <interpreter/interval.h>
#include <interpreter/memory.h>
#include <libc/libc.h>
#include <utility.h>
#include <memory>

namespace dark {

struct ICache {
    explicit ICache(Memory &);
    auto ifetch(target_size_t, Hint) noexcept -> Executable &;
private:
    const std::size_t length;   // Command length
    std::unique_ptr <Executable[]> cached;
};

} // namespace dark
