#include "interpreter/register.h"
#include "config/config.h"
#include "utility/error.h"

namespace dark {

using dark::console::profile;

RegisterFile::RegisterFile(target_size_t entry, const Config &config) :
    regs(), pc(), new_pc(entry) {
    (*this)[Register::sp] = config.get_stack_top();
    (*this)[Register::ra] = this->end_pc;
}

void RegisterFile::print_details(bool detail) const {
    const auto exit_code = this->regs[static_cast<int>(Register::a0)];
    profile << fmt::format("Exit code: {}\n", exit_code);

    if (!detail)
        return;

    for (std::size_t i = 0; i < this->regs.size(); ++i)
        profile << fmt::format(
            "- {:<4} = {:#08x}\n", reg_to_sv(static_cast<Register>(i)), this->regs[i]
        );
}

} // namespace dark
