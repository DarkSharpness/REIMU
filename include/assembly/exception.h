#pragma once
#include "fmtlib.h"
#include <string>

namespace dark {

struct FailToParse {
    std::string inner;
};

template <typename _Tp, typename... _Args>
__attribute((always_inline)) inline static void
throw_if(_Tp &&condition, fmt::format_string<_Args...> fmt = "", _Args &&...args) {
    if (condition)
        throw FailToParse(fmt::format(fmt, std::forward<_Args>(args)...));
}

} // namespace dark
