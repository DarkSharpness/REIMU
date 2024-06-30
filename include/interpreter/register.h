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
    target_size_t &operator[](Register reg) {
        return this->regs[static_cast<std::size_t>(reg)];
    }
    target_size_t &get_pc() { return this->pc; }
    void set_pc(target_size_t value) { this->pc = value; }
};

} // namespace dark
