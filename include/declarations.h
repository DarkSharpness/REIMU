#pragma once
#include <iosfwd>
#include <cstdint>

namespace dark {

struct Config;
struct Parser;
struct Linker;
struct Assembler;
struct Interpreter;
struct Storage {
    virtual ~Storage() noexcept = default;
    virtual void debug(std::ostream&) const = 0;
};

using target_size_t = std::uint32_t;

} // namespace dark
