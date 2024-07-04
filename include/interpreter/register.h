#pragma once
#include <declarations.h>
#include <riscv/register.h>
#include <array>

namespace dark {

struct RegisterFile {
  private:
    std::array <target_size_t, 32> regs;

    target_size_t pc;
    target_size_t new_pc;

  public:
    explicit RegisterFile() = default;
    /* Return reference to given register. */
    auto &operator[](Register reg) {
        return this->regs[static_cast<std::size_t>(reg)];
    }
    /* Return old program counter. */
    auto get_pc() { return this->pc; }
    /* Set new program counter. */
    void set_pc(target_size_t pc) { this->new_pc = pc; }
    /* Complete after one cycle. */
    auto advance() { this->pc = this->new_pc; this->new_pc += 4; return this->pc; }
};

} // namespace dark
