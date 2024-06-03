#pragma once
#include "declarations.h"
#include "assembly.h"

namespace dark {

struct Parser {
    std::vector <Assembly> files;
    explicit Parser(const Config &);
};

} // namespace dark
