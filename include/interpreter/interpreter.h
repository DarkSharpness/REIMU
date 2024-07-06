#pragma once
#include <declarations.h>
#include <linker/memory.h>

namespace dark {

struct Interpreter {
    MemoryLayout layout;
    explicit Interpreter(const Config &);
    void interpret(const Config &config);
};

} // namespace dark
