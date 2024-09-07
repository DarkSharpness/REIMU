#pragma once
#include "riscv/register.h"
#include <optional>

namespace dark {

struct AssemblyLayout;

struct Assembly;

struct ImmediateBase {
    virtual ~ImmediateBase() = default;
};

struct Immediate;

struct StorageVisitor;
struct Storage {
    virtual ~Storage() noexcept              = default;
    virtual void debug(std::ostream &) const = 0;
    virtual void accept(StorageVisitor &)    = 0;
};

auto is_label_char(char) -> bool;
auto sv_to_reg(std::string_view) -> Register;
auto sv_to_reg_nothrow(std::string_view) noexcept -> std::optional<Register>;

} // namespace dark
