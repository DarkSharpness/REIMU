#include <interpreter/device.h>
#include <simulation/predictor.h>
#include <config/config.h>
#include <utility.h>
#include <utility/reflect.h>
#include <optional>

namespace dark {

using console::profile;

// Some hidden implementation data.
struct Device_Impl {
    std::size_t bp_failed;
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
            .bp_failed = {},
            .bp = {},
            .config = config,
        }
    {
        if (config.has_option("predictor")) bp.emplace();
    }
};

auto Device::create(const Config &config)
-> std::unique_ptr <Device> {
    return std::unique_ptr <Device> (new Device::Impl {config});
}

void Device::predict(target_size_t pc, bool what) {
    auto &impl  = this->get_impl();
    if (!impl.bp.has_value()) return;
    auto &bp    = *impl.bp;
    auto result = bp.predict(pc);
    impl.bp_failed += (result != what);
    bp.update(pc, what);
}

Device::~Device() {
    std::destroy_at <Device_Impl> (&this->get_impl());
}

template <typename _Tp>
requires std::is_aggregate_v <_Tp>
static auto sum_members(const _Tp &obj) -> std::size_t {
    const auto &tuple = reflect::tuplify(obj);
    return std::apply([](auto &...args) {
        return (args + ...);
    }, tuple); 
}

auto Device::get_impl() -> Impl & {
    return *static_cast <Impl *> (this);
}

void Device::print_details(bool details) const {
    // if (!details) return;
    allow_unused(details);
    auto &impl = *static_cast <const Impl *> (this);

    const std::size_t arith =
        impl.counter.add * impl.config.get_weight("add")
    +   impl.counter.sub * impl.config.get_weight("sub")
    +   impl.counter.lui * impl.config.get_weight("lui")
    +   impl.counter.slt * impl.config.get_weight("slt")
    +   impl.counter.sltu * impl.config.get_weight("sltu")
    +   impl.counter.auipc * impl.config.get_weight("auipc");

    const std::size_t logic =
        impl.counter.and_ * impl.config.get_weight("and")
    +   impl.counter.or_ * impl.config.get_weight("or")
    +   impl.counter.xor_ * impl.config.get_weight("xor")
    +   impl.counter.sll * impl.config.get_weight("sll")
    +   impl.counter.srl * impl.config.get_weight("srl")
    +   impl.counter.sra * impl.config.get_weight("sra");

    const std::size_t mem =
        impl.counter.lb * impl.config.get_weight("lb")
    +   impl.counter.lh * impl.config.get_weight("lh")
    +   impl.counter.lw * impl.config.get_weight("lw")
    +   impl.counter.lbu * impl.config.get_weight("lbu")
    +   impl.counter.lhu * impl.config.get_weight("lhu")
    +   impl.counter.sb * impl.config.get_weight("sb")
    +   impl.counter.sh * impl.config.get_weight("sh")
    +   impl.counter.sw * impl.config.get_weight("sw");

    const std::size_t branch =
        impl.counter.beq * impl.config.get_weight("beq")
    +   impl.counter.bne * impl.config.get_weight("bne")
    +   impl.counter.blt * impl.config.get_weight("blt")
    +   impl.counter.bge * impl.config.get_weight("bge")
    +   impl.counter.bltu * impl.config.get_weight("bltu")
    +   impl.counter.bgeu * impl.config.get_weight("bgeu");

    const std::size_t multiply =
        impl.counter.mul * impl.config.get_weight("mul")
    +   impl.counter.mulh * impl.config.get_weight("mulh")
    +   impl.counter.mulhsu * impl.config.get_weight("mulhsu")
    +   impl.counter.mulhu * impl.config.get_weight("mulhu");

    const std::size_t divide =
        impl.counter.div * impl.config.get_weight("div")
    +   impl.counter.divu * impl.config.get_weight("divu")
    +   impl.counter.rem * impl.config.get_weight("rem")
    +   impl.counter.remu * impl.config.get_weight("remu");

    const std::size_t jump = impl.counter.jal * impl.config.get_weight("jal");
    const std::size_t jalr = impl.counter.jalr * impl.config.get_weight("jalr");

    profile << std::format(
        "Total cycles: {}\n",
        arith + logic + mem + branch + jump + jalr + multiply + divide
    );

    using namespace config;

    profile << std::format("Instruction parsed: {}\n", impl.counter.iparse);

    profile << std::format(
        "Instruction counts:\n"
        "# arith  = {}\n"
        "# mul    = {}\n"
        "# div    = {}\n"
        "# bits   = {}\n"
        "# mem    = {}\n"
        "# branch = {}\n"
        "# jump   = {}\n"
        "# jalr   = {}\n",
        sum_members <CounterArith> (impl.counter),
        sum_members <CounterMultiply> (impl.counter),
        sum_members <CounterDivide> (impl.counter),
        sum_members <CounterBitwise> (impl.counter),
        sum_members <CounterMemory> (impl.counter),
        sum_members <CounterBranch> (impl.counter),
        impl.counter.jal, impl.counter.jalr
    );

    if (impl.bp.has_value()) {
        if (auto total = sum_members <CounterBranch> (impl.counter)) {
            auto failed = impl.bp_failed;
            profile << std::format(
                "Branch predictior failures: {:.2f}% ({}/{}) \n",
                failed * 100.0 / total, failed, total
            );
        }
    }
}

} // namespace dark
