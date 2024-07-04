#pragma once
#include <declarations.h>
#include <riscv/register.h>
#include <array>

namespace dark {

struct RegisterFile {
  private:
    std::array <target_size_t, 32> regs;
    target_size_t pc;
  public:
    explicit RegisterFile() = default;
    /* Return reference to given register. */
    target_size_t &operator[](Register reg) {
        return this->regs[static_cast<std::size_t>(reg)];
    }
    /* Return reference to program counter. */
    target_size_t &get_pc() { return this->pc; }
};

} // namespace dark
