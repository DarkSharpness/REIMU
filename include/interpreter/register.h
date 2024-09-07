#pragma once
#include "declarations.h"
#include "riscv/register.h"
#include <array>

namespace dark {

struct RegisterFile {
private:
    std::array<target_size_t, 32> regs;

    target_size_t pc;
    target_size_t new_pc;

    static constexpr target_size_t end_pc = 0x4;

public:
    /* Constructor. */
    explicit RegisterFile(target_size_t, const Config &);
    /* Return reference to given register. */
    auto &operator[](Register reg) { return this->regs[reg_to_int(reg)]; }
    /* Return value of given register. */
    auto operator[](Register reg) const { return this->regs[reg_to_int(reg)]; }
    /* Return old program counter. */
    auto get_pc() const { return this->pc; }
    /* Set new program counter. */
    void set_pc(target_size_t pc) { this->new_pc = pc; }
    /* Complete after one instruction. */
    bool advance() {
        this->pc                = this->new_pc;
        this->new_pc            = this->pc + sizeof(command_size_t);
        (*this)[Register::zero] = 0;
        return this->pc != this->end_pc;
    }
    /* Print register details. */
    void print_details(bool) const;
    /* Psuedo start pc (pretend there's a call at get_start_pc). */
    static constexpr auto get_start_pc() { return end_pc - 4; }
};

} // namespace dark
