#include <utility.h>
#include <interpreter/exception.h>
#include <interpreter/register.h>
#include <interpreter/memory.h>
#include <interpreter/device.h>
#include <libc/libc.h>

namespace dark {

auto FailToInterpret::what(RegisterFile &rf, Memory &mem, Device &dev) const -> std::string {
    const auto __misaligned = [this](auto &&type) {
        return std::format("{} misaligned at 0x{:x} | alignment = {}",
                type, this->address, this->alignment);
    };
    const auto __outofbound = [this](auto &&type) {
        return std::format("{} out of bound at 0x{:x} | size = {}",
            type, this->address, this->size);
    };
    const auto __libc_name = [this] {
        return std::format("libc::{}", libc::names[this->libc_which]);
    };

    switch (this->error) {
        using enum Error;
        case LoadMisAligned:
            return __misaligned("Load");
        case StoreMisAligned:
            return __misaligned("Store");
        case InsMisAligned:
            return __misaligned("Ins-fetch");
        case LibcMisAligned:
            return __misaligned(__libc_name());

        case LoadOutOfBound:
            return __outofbound("Load");
        case StoreOutOfBound:
            return __outofbound("Store");
        case InsOutOfBound:
            return __outofbound("Ins-fetch");
        case LibcOutOfBound:
            return __outofbound(__libc_name());

        case InsUnknown:
            return std::format("Unknown instruction at 0x{:x}: 0x{}",
                rf.get_pc(), this->command);

        case DivideByZero:
            return std::format("Divide by zero at 0x{:x}", rf.get_pc());

        case NotImplemented:
            return "Not implemented";


        default:
            unreachable();
    }
}


} // namespace dark
