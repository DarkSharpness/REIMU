#include "config/counter.h"
#include "utility/tagged.h"
#include <cstddef>
#include <interpreter/device.h>
#include <simulation/predictor.h>
#include <config/config.h>
#include <utility.h>
#include <optional>

namespace dark {

using console::profile;

// Some hidden implementation data.
struct Device_Impl {
    std::size_t bp_success;
    std::optional <BranchPredictor> bp;
    const Config &config;
};

struct Device::Impl : Device, Device_Impl {
    explicit Impl(const Config &config)
        : Device {
            .counter = {},
            .in = config.get_input_stream(),
            .out = config.get_output_stream(),
        }, Device_Impl {
            .bp_success = 0,
            .bp = {},
            .config = config,
        }
    {
        if (config.has_option("predictor")) bp.emplace();
    }
};

auto Device::create(const Config &config) -> unique_t {
    return unique_t { new Device::Impl {config} };
}

void Device::predict(target_size_t pc, bool what) {
    auto &impl  = this->get_impl();
    if (!impl.bp.has_value()) return;
    auto &bp    = *impl.bp;
    auto result = bp.predict(pc);
    impl.bp_success += (result == what);
    bp.update(pc, what);
}

static auto operator *(
    const weight::Counter &lhs,
    const weight::Counter &rhs    
) -> std::size_t {
    std::size_t sum = 0;
    visit(
        [&](auto &lhs, auto &rhs) {
            sum += lhs.get_weight() * rhs.get_weight();
        }, lhs, rhs
    );
    return sum;
}

auto Device::get_impl() -> Impl & {
    return *static_cast <Impl *> (this);
}

void Device::print_details(bool details) const {
    // if (!details) return;
    allow_unused(details);
    auto &impl = *static_cast <const Impl *> (this);

    const auto &kWeight = impl.config.get_weight();
    const auto &counter = impl.counter;

    // Original cycle count.
    auto cycles = counter * kWeight;
    if (impl.bp.has_value()) {
        cycles -= impl.bp_success * kWeight.wBranch;
        cycles += impl.bp_success * kWeight.wPredictTaken;
    }

    profile << std::format("Total cycles: {}\n", cycles);
    profile << std::format("Instruction parsed: {}\n", impl.counter.iparse);

    profile << std::format(
        "Instruction counts:\n"
        "# simple = {}\n"
        "# mul    = {}\n"
        "# div    = {}\n"
        "# mem    = {}\n"
        "# branch = {}\n"
        "# jump   = {}\n"
        "# jalr   = {}\n",
        counter.wArith + counter.wUpper + counter.wCompare +
        counter.wShift + counter.wBitwise,
        counter.wMultiply,
        counter.wDivide,
        counter.wLoad + counter.wStore,
        counter.wBranch,
        counter.wJal,
        counter.wJalr
    );

    if (impl.bp.has_value()) {
        if (auto total = counter.wBranch) {
            profile << std::format(
                "Branch prediction success rate: {:.2f}%\n",
                100.0 * impl.bp_success / total
            );
        }
    }
}

} // namespace dark
