#include <utility.h>
#include <interpreter/exception.h>
#include <interpreter/register.h>
#include <interpreter/memory.h>
#include <interpreter/device.h>

namespace dark {

auto FailToInterpret::what(RegisterFile &rf, Memory &mem, Device &dev) const -> std::string {
    const auto __misaligned = [&](std::string_view type) {
        return std::format("{} misaligned at {:x}, required alignment: {}",
                type, this->address, this->alignment);
    };
    const auto __outofbound = [&](std::string_view type) {
        return std::format("{} out of bound at {:x}", type, this->address);
    };

    switch (this->error) {
        using enum Error;
        case LoadMisAligned:
            return __misaligned("Load");
        case StoreMisAligned:
            return __misaligned("Store");
        case InsMisAligned:
            return __misaligned("Instruction");

        case LoadOutOfBound:
            return __outofbound("Load");
        case StoreOutOfBound:
            return __outofbound("Store");
        case InsOutOfBound:
            return __outofbound("Instruction");

        case InsUnknown:
            return std::format("Unknown instruction at {:x}", rf.get_pc());

        case DivideByZero:
            return std::format("Divide by zero at {:x}", rf.get_pc());

        case NotImplemented:
            return "Not implemented";

        default:
            unreachable();
    }
}


} // namespace dark
