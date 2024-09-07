#include "interpreter/device.h"
#include "config/config.h"
#include "config/counter.h"
#include "declarations.h"
#include "simulation/dcache.h"
#include "simulation/predictor.h"
#include "utility/misc.h"
#include "utility/tagged.h"
#include <cstddef>
#include <optional>

namespace dark {

using console::profile;

static auto operator*(const weight::Counter &lhs, const weight::Counter &rhs) -> std::size_t {
    std::size_t sum = 0;
    visit([&](auto &lhs, auto &rhs) { sum += lhs.get_weight() * rhs.get_weight(); }, lhs, rhs);
    return sum;
}

// Some hidden implementation data.
struct Device_Impl {
    std::size_t bp_success;
    std::size_t cache_load;
    std::size_t cache_store;
    const Config &config;
    std::optional<BranchPredictor> bp;
    std::optional<kupi::Cache> cache;
};

struct Device::Impl : Device, Device_Impl {
    explicit Impl(const Config &config) :
        Device{
            .counter = {},
            .in      = config.get_input_stream(),
            .out     = config.get_output_stream(),
        },
        Device_Impl{
            .bp_success  = 0,
            .cache_load  = 0,
            .cache_store = 0,
            .config      = config,
            .bp          = {},
            .cache       = {},
        } {
        if (config.has_option("predictor"))
            bp.emplace();
        if (config.has_option("cache"))
            cache.emplace();
    }
};

auto Device::create(const Config &config) -> unique_t {
    return unique_t{new Device::Impl{config}};
}

void Device::predict(target_size_t pc, bool what) {
    if (auto &impl = this->get_impl(); impl.bp.has_value()) {
        auto &bp    = *impl.bp;
        auto result = bp.predict(pc);
        impl.bp_success += (result == what);
        bp.update(pc, what);
    }
}

void Device::try_load(target_size_t addr, target_size_t size) {
    if (auto &impl = this->get_impl(); impl.cache.has_value()) {
        auto &cache = *impl.cache;
        impl.cache_load += cache.load(addr, addr + size);
    }
}

void Device::try_store(target_size_t addr, target_size_t size) {
    if (auto &impl = this->get_impl(); impl.cache.has_value()) {
        auto &cache = *impl.cache;
        impl.cache_store += cache.store(addr, addr + size);
    }
}

auto Device::get_impl() -> Impl & {
    return *static_cast<Impl *>(this);
}

void Device::print_details(bool details) const {
    allow_unused(details);
    auto &impl = *static_cast<const Impl *>(this);

    const auto &kWeight = impl.config.get_weight();
    const auto &counter = impl.counter;

    // Original cycle count.
    auto cycles = counter * kWeight;
    cycles += counter.libcMem.weight + counter.libcIO.weight + counter.libcOp.weight;

    if (impl.bp.has_value()) {
        cycles -= impl.bp_success * kWeight.wBranch;
        cycles += impl.bp_success * kWeight.wPredictTaken;
    }

    if (impl.cache.has_value()) {
        cycles -= counter.wLoad * kWeight.wLoad;
        cycles -= counter.wStore * kWeight.wStore;
        const auto load  = impl.cache->get_load();
        const auto store = impl.cache->get_store();

        cycles += load * kWeight.wLoad;
        cycles += store * kWeight.wStore;
        cycles += impl.cache_load * kWeight.wCacheLoad;
        cycles += impl.cache_store * kWeight.wCacheStore;
    }

    profile << fmt::format("Total cycles: {}\n", cycles);
    profile << fmt::format("Instruction parsed: {}\n", impl.counter.iparse);

    profile << fmt::format(
        "Instruction counts:\n"
        "# simple   = {}\n"
        "# mul      = {}\n"
        "# div      = {}\n"
        "# mem      = {}\n"
        "# branch   = {}\n"
        "# jump     = {}\n"
        "# jalr     = {}\n"
        "# libcMem  = {}\n"
        "# libcIO   = {}\n"
        "# libcOp   = {}\n",
        counter.wArith + counter.wUpper + counter.wCompare + counter.wShift + counter.wBitwise,
        counter.wMultiply,              // multiply
        counter.wDivide,                // divide
        counter.wLoad + counter.wStore, // memory
        counter.wBranch,                // branch
        counter.wJal,                   // jump
        counter.wJalr,                  // jalr
        counter.libcMem.count,          // libcMem
        counter.libcIO.count,           // libcIO
        counter.libcOp.count            // libcOp
    );

    if (impl.bp.has_value()) {
        if (auto total = counter.wBranch) {
            profile << fmt::format(
                "Branch prediction taken rate: {:.2f}% ({}/{})\n", 100.0 * impl.bp_success / total,
                impl.bp_success, total
            );
        }
    }

    if (impl.cache.has_value()) {
        if (auto total = counter.wLoad + counter.wStore) {
            profile << fmt::format(
                "Cache hit rate: {:.2f}% ({}/{})\n",
                100.0 * (impl.cache_load + impl.cache_store) / total,
                impl.cache_load + impl.cache_store, total
            );
        }
    }
}

} // namespace dark
