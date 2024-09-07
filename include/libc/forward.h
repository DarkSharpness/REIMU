#pragma once
#include "declarations.h"
#include "riscv/abi.h"

namespace dark::libc {

using libc_index_t               = std::uint16_t;
static constexpr auto kLibcStart = kTextStart;

} // namespace dark::libc
