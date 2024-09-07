#include "assembly/assembly.h"
#include "assembly/layout.h"
#include "config/config.h"
#include "interpreter/interpreter.h"

namespace dark {

void Interpreter::assemble() {
    std::vector<AssemblyLayout> layouts;

    layouts.reserve(config.get_assembly_names().size());
    for (const auto &file : config.get_assembly_names()) {
        Assembler assembler{file};
        layouts.emplace_back(assembler.get_standard_layout());
    }

    this->assembly_layout = std::move(layouts);
}

} // namespace dark
