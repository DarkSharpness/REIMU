#include <simulation/implement/predictor_decl.h>
#include <random>

namespace dark {

template <std::size_t _Div>
static auto div_mod(target_size_t index) {
    return std::make_pair(index / _Div, index % _Div);
}

branch_predictor::branch_predictor() : table() {
    // Fill the table with random bits
    std::random_device rd;
    std::mt19937 gen(rd());
    for (auto &entry : this->table) entry = gen();
}

auto branch_predictor::predict(target_size_t pc) const -> bool {
    auto index = this->get_index(pc);
    constexpr auto kHalf = kMask >> 1;
    return this->get_bits(index) > kHalf;
}

auto branch_predictor::update(target_size_t pc, bool taken) -> void {
    const auto index    = this->get_index(pc);
    const auto data     = this->get_bits(index);

    constexpr auto kMax = kMask;
    constexpr auto kMin = target_size_t {0};

    if (taken) {
        if (data != kMax)
            this->set_bits(index, (data + 1) ^ data);
    } else {
        if (data != kMin)
            this->set_bits(index, (data - 1) ^ data);
    }
}

auto branch_predictor::get_index(target_size_t pc) -> target_size_t {
    static_assert(sizeof(command_size_t) == 4);
    return ((pc / sizeof(command_size_t)) & (_Nm - 1)) * kBits;
}

auto branch_predictor::get_bits(target_size_t index) const -> target_size_t {
    const auto [which, offset] = div_mod <kDigit> (index);
    return (this->table[which] >> offset) & kMask;
}

auto branch_predictor::set_bits(target_size_t index, target_size_t delta) -> void {
    const auto [which, offset] = div_mod <kDigit> (index);
    this->table[which] ^= (delta << offset);
}

} // namespace dark
