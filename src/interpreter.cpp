#include "interpreter.h"
#include "config.h"
#include "assembly.h"

namespace dark {

Interpreter::Interpreter(const Config &config) {
    std::vector <Assembly> assemblies;
    assemblies.reserve(config.assembly_files.size());
    for (const auto &file : config.assembly_files)
        assemblies.emplace_back(file); 
}


} // namespace dark
