#pragma once
#include "declarations.h"
#include "utility/any.h"

namespace dark {

struct Interpreter {
public:
    explicit Interpreter(const Config &config) : config(config) {}

    void assemble();
    void link();
    void simulate();

private:
    const Config &config;
    any assembly_layout;
    any memory_layout;
};

} // namespace dark
