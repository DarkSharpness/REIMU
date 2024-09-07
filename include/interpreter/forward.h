#pragma once
#include "declarations.h"

// Some forward declarations to avoid circular dependencies
namespace dark {

struct Memory;
struct Device;
struct Executable;
struct RegisterFile;
struct Interval;
struct Hint;

using Function_t = Hint(Executable &, RegisterFile &, Memory &, Device &);

} // namespace dark
