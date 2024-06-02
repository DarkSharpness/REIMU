#pragma once
#include "utility.h"
#include "assembly.h"
#include <fstream>

namespace dark {

struct Parser {
    std::vector <Assembly> files;
    Parser(const Config &config);
};

} // namespace dark
