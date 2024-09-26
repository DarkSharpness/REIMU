#pragma once
#include "riscv/register.h"
#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>

namespace dark {

struct AssemblyLayout;

struct Assembly;

struct ImmediateBase {
    virtual ~ImmediateBase() = default;
};

struct Immediate;

struct StorageVisitor;
struct Storage {
    struct LineInfo {
        std::shared_ptr<char[]> file;
        std::size_t line;
        auto to_string() const -> std::string { return std::string(file.get()); }
    };
    LineInfo line_info;
    Storage(LineInfo li) noexcept : line_info(li) {}
    virtual ~Storage() noexcept              = default;
    virtual void debug(std::ostream &) const = 0;
    virtual void accept(StorageVisitor &)    = 0;
};

auto is_label_char(char) -> bool;
auto sv_to_reg(std::string_view) -> Register;
auto sv_to_reg_nothrow(std::string_view) noexcept -> std::optional<Register>;
[[noreturn]]
auto handle_build_failure(std::string msg, const std::string &file_name, std::size_t line) -> void;

} // namespace dark
