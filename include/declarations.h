#pragma once
#include <iosfwd>
#include <cstdint>

namespace dark {

struct Config;
struct Linker;
struct Assembler;
struct Interpreter;

enum class Section : std::uint8_t {
    UNKNOWN, TEXT, DATA, RODATA, BSS,
    MAXCOUNT = 5
};

using command_size_t = std::uint32_t;
using target_size_t  = std::uint32_t;
using target_ssize_t = std::int32_t;

static_assert(sizeof(target_size_t) == sizeof(target_ssize_t));

} // namespace dark
