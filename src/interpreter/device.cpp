#include <interpreter/device.h>
#include <config/config.h>

namespace dark {

// Some hidden implementation data.
struct Device_Impl {
};

struct Device::Impl : Device_Impl, Device {
    explicit Impl(const Config &config)
        : Device_Impl {
        }, Device {
            .counter = {},
            .in = config.get_input_stream(),
            .out = config.get_output_stream(),
        } {}
};

auto Device::create(const Config &config)
-> std::unique_ptr <Device> {
    return std::unique_ptr <Device> (new Device::Impl {config});
}

void Device::predict(target_size_t, bool) {
    // Do nothing now.
}

Device::~Device() {
    std::destroy_at <Device_Impl> (&this->get_impl());
}

auto Device::get_impl() -> Impl & {
    return *static_cast <Impl *> (this);
}

void Device::print_details(bool detail) const {
    
}

} // namespace dark
