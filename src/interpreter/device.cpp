#include <interpreter/device.h>
#include <utility/config.h>

namespace dark {

auto Device::create(const Config &config, std::istream &in, std::ostream &out)
-> std::unique_ptr <Device> {
    return std::make_unique <Device> (Counter {}, in, out);
}

void Device::predict(target_size_t, bool) {
    // Do nothing now.
}

} // namespace dark
