#include "declarations.h"
#include <limits>
#include <array>

namespace dark {

/**
 * A simple 2 bit branch predictor.
 */
// template <std::size_t _Nm = 4096>
struct BranchPredictor {
private:
    using _Data_t = std::uint8_t;

    static constexpr target_size_t _Nm    = 4096;
    static constexpr target_size_t kBits  = 2;
    static constexpr target_size_t kMask  = (1 << kBits) - 1;

    static constexpr target_size_t kDigit = std::numeric_limits<_Data_t>::digits;
    static constexpr target_size_t kBytes = (_Nm * kBits) / kDigit;

    static_assert(kBytes * kDigit == _Nm * kBits, "Invalid size");

    std::array <_Data_t, kBytes> table;

    static auto get_index(target_size_t) -> target_size_t;

    auto get_bits(target_size_t) const -> target_size_t;
    auto set_bits(target_size_t, target_size_t) -> void;

public:
    BranchPredictor();
    bool predict(target_size_t) const;
    void update(target_size_t, bool);
};

} // namespace dark
