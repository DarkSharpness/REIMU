#include <config.h>
#include <interpreter/interpreter.h>
#include <assembly/assembly.h>
#include <linker/linker.h>

namespace dark {

Interpreter::Interpreter(const Config &config) {
    std::vector <Assembler> assemblies;
    assemblies.reserve(config.assembly_files.size());
    for (const auto &file : config.assembly_files)
        assemblies.emplace_back(file);

    Linker linker { assemblies };
}


} // namespace dark
