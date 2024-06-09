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

enum class Section : std::uint8_t {
    UNKNOWN, TEXT, DATA, RODATA, BSS,
    MAXCOUNT = 5
};

using target_size_t = std::uint32_t;
using target_ssize_t = std::int32_t;

} // namespace dark
