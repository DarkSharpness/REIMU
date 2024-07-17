#pragma once
#include <declarations.h>
#include <riscv/register.h>
/// TODO: remove this header
#include <memory>

namespace dark {

struct AssemblyLayout;

struct Assembly;

struct ImmediateBase {
    virtual ~ImmediateBase() = default;
};
struct Immediate;

struct StorageVisitor;
struct Storage {
    virtual ~Storage() noexcept = default;
    virtual void debug(std::ostream&) const = 0;
    virtual void accept(StorageVisitor&) = 0;
};

auto is_label_char(char) -> bool;
auto sv_to_reg(std::string_view) -> Register;

} // namespace dark