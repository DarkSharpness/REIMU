#pragma once
#include <declarations.h>

// Some forward declarations to avoid circular dependencies
namespace dark {

struct Memory;
struct Device;
struct Executable;
struct RegisterFile;

namespace handle {

[[noreturn]] target_size_t div_by_zero();
[[noreturn]] target_size_t unknown_instruction(std::uint32_t);

} // namespace handle

} // namespace dark
