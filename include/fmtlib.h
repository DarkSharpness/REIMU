#pragma once
#include <version>

#ifdef __cpp_lib_format

#include <format>

namespace fmt {

using std::format;
using std::format_string;

} // namespace fmt

#else

#include <fmt/format.h>

#endif
