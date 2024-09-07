#include "interpreter/exception.h"
#include "fmtlib.h"
#include "interpreter/device.h"
#include "interpreter/memory.h"
#include "interpreter/register.h"
#include "libc/libc.h"
#include "utility/error.h"

namespace dark {

auto FailToInterpret::what(RegisterFile &rf, Memory &, Device &) const -> std::string {
    const auto __misaligned = [this](auto &&type) {
        return fmt::format(
            "{} misaligned at 0x{:x} | alignment = {}", type, this->detail.address,
            this->detail.alignment
        );
    };
    const auto __outofbound = [this](auto &&type) {
        return fmt::format(
            "{} out of bound at 0x{:x} | size = {}", type, this->detail.address, this->detail.size
        );
    };
    const auto __libc_name = [this] {
        return fmt::format("libc::{}", libc::names[this->libc_which]);
    };

    switch (this->error) {
        using enum Error;
        case LoadMisAligned:  return __misaligned("Load");
        case StoreMisAligned: return __misaligned("Store");
        case InsMisAligned:   return __misaligned("Ins-fetch");
        case LibcMisAligned:  return __misaligned(__libc_name());

        case LoadOutOfBound:  return __outofbound("Load");
        case StoreOutOfBound: return __outofbound("Store");
        case InsOutOfBound:   return __outofbound("Ins-fetch");
        case LibcOutOfBound:  return __outofbound(__libc_name());

        case InsUnknown:
            return fmt::format(
                "Unknown instruction at 0x{:x}: 0x{:x}", rf.get_pc(), this->detail.command
            );

        case DivideByZero:   return fmt::format("Divide by zero at 0x{:x}", rf.get_pc());
        case LibcError:      return fmt::format("{}: {}", __libc_name(), this->message);
        case NotImplemented: return "Not implemented";

        default:             unreachable();
    }
}


} // namespace dark
