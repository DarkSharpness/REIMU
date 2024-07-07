#pragma once
#include <declarations.h>
#include <string_view>

namespace dark::config {

static constexpr std::size_t kInitMemorySize    = 256 * 1024 * 1024;
static constexpr std::size_t kInitStackSize     = 32 * 1024;
static constexpr std::size_t kInitTimeOut       = static_cast<std::size_t>(-1);
static constexpr std::string_view kInitStdin    = "<stdin>";
static constexpr std::string_view kInitStdout   = "<stdout>";

static constexpr std::string_view kSupportedOptions[] = {
    "cache",
    "debug",
    "detail",
    "silent",
};

static constexpr std::string_view kInitAssemblyFiles[] = {
    "builtin.s",
    "test.s",
};

} // namespace dark::config
