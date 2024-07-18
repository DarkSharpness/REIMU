#include <config/config.h>
#include <interpreter/memory.h>
#include <interpreter/device.h>
#include <interpreter/register.h>
#include <fmtlib>
#include <iostream>
#include <ranges>

namespace dark {

RegisterFile::RegisterFile(target_size_t entry, const Config &config)
    : regs(), pc(), new_pc(entry) {
    (*this)[Register::sp] = config.get_stack_top();
    (*this)[Register::ra] = this->end_pc;
}

void RegisterFile::print_details(bool detail) const {
    const auto exit_code = this->regs[static_cast<int>(Register::a0)];
    std::cout << std::format("Exit code: {}\n", exit_code);

    if (!detail) return;

    for (std::size_t i = 0 ; i < this->regs.size() ; ++i)
        std::cout << std::format("- {:<4} = 0x{:08x}\n",
            reg_to_sv(static_cast<Register>(i)), this->regs[i]);
}

} // namespace dark
