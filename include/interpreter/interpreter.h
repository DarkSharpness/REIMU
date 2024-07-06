#pragma once
#include <declarations.h>
#include <linker/layout.h>

namespace dark {

struct Interpreter {
    MemoryLayout layout;
    explicit Interpreter(const Config &);
    void interpret(const Config &config);
};

} // namespace dark
