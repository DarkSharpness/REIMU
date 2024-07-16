#pragma once
#include <string>
#include <fmtlib>

namespace dark {

struct FailToParse { std::string inner; };

template <typename _Tp, typename ..._Args>
__attribute((always_inline))
inline static void throw_if(_Tp &&condition, std::format_string <_Args...> fmt = "", _Args &&...args) {
    if (condition) throw FailToParse(std::format(fmt, std::forward <_Args>(args)...));
}

} // namespace dark
