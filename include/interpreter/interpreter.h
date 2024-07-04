#pragma once
#include <declarations.h>

namespace dark {

struct MemoryLayout;

struct Interpreter {
    explicit Interpreter(const Config &);

    static void interpret(const Config &config, MemoryLayout source);
};

} // namespace dark
