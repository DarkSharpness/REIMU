#include <interpreter/executable.h>
#include <interpreter/interval.h>
#include <interpreter/memory.h>
#include <libc/libc.h>
#include <utility.h>
#include <memory>

namespace dark {

struct ICache {
    using Ref_t = std::reference_wrapper<Executable>;
    explicit ICache(Memory &);
    auto ifetch(target_size_t, Hint) noexcept -> Ref_t;
  private:
    target_size_t length;
    std::unique_ptr <Executable[]> cached;
};

} // namespace dark
