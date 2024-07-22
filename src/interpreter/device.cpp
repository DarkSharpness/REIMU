#include <interpreter/device.h>
#include <simulation/predictor.h>
#include <config/config.h>
#include <utility.h>

namespace dark {

// Some hidden implementation data.
struct Device_Impl {
    std::size_t failed = 0;
    branch_predictor bp;
};

struct Device::Impl : Device, Device_Impl {
    explicit Impl(const Config &config)
        : Device {
            .counter = {},
            .in = config.get_input_stream(),
            .out = config.get_output_stream(),
        }, Device_Impl {}
        {}
};

auto Device::create(const Config &config)
-> std::unique_ptr <Device> {
    return std::unique_ptr <Device> (new Device::Impl {config});
}

void Device::predict(target_size_t pc, bool what) {
    auto &impl  = this->get_impl();
    auto &bp    = impl.bp;
    auto result = bp.predict(pc);
    impl.failed += (result != what);
    bp.update(pc, what);
}

Device::~Device() {
    std::destroy_at <Device_Impl> (&this->get_impl());
}

auto Device::get_impl() -> Impl & {
    return *static_cast <Impl *> (this);
}

void Device::print_details(bool details) const {
    // if (!details) return;
    allow_unused(details);
    auto &impl = *static_cast <const Impl *> (this);
    auto failed = impl.failed;
    auto total = impl.counter.beq + impl.counter.bne
        + impl.counter.bgeu + impl.counter.bltu
        + impl.counter.bge + impl.counter.blt;

    std::cout << std::format(
        "Branch Prediction: {}, Failed: {}, Failure Rate: {:.2f}%\n",
        total, failed, failed * 100.0 / total
    );
}

} // namespace dark
