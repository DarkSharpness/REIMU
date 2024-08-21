# Extension

## Overview

This project aims to enhance the scalability and readability of the simulator.

The simulator is primarily composed of four main components:

- Config Parser
- Frontend (Assembler)
- Midend (Linker)
- Backend (Interpreter)

### Config Parser

The Config Parser defines the simulator's configuration, which significantly influences all other components.

### Major Components

The Frontend, Midend, and Backend are designed to operate almost independently. They communicate via the `layout` class, which serves as the abstract data holder for each preceding stage. This structure is intended to improve modularity and maintainability, making the codebase easier to extend and understand.

#### Assembler

The Assembler focuses on two key concepts: storage and value.

- **Storage:** Can represent an instruction (e.g., `addi a0, a0, 114`) or static data (e.g., `.align 4`, `.word 1, 2, 3`).
- **Value:** Can be an immediate type or a register. The `Immediate` type, defined in `include/assembly/storage/immediate.h`, maintains a tree-like structure (see `include/assembly/immediate.h`). The `Register` type, defined in `include/riscv/register.h`, is used in the Assembler, Linker, and Interpreter.

A value can appear in storage, as shown below:

```cpp
// jal (jump and link register), see `include/assembly/storage/command.h`
struct JumpRelative final : Command {
    Register rd;
    Immediate imm;
};
```

In this section, we use our pattern matching library (`include/assembly/frontend`) to parse storage.

We have a lexer that converts the input `string_view` into a `TokenStream`. The `dark::frontend::match` function helps parse the input string. Any match failure will throw an error.

```cpp
using frontend::match;
using frontend::Token::Placeholder;

using Reg = Register;
using Imm = Immediate;
using OffReg = frontend::OffsetRegister;

// Usage:
// Match some value types, separated by colons, and return a tuple.
// Support a Placeholder at the end, indicating there is more to come.
// If matching fails, it throws an error.

// Example: Matches values like zero, main + 10, 12(sp), ...
auto [reg, imm, off, _] = match <Reg, Imm, OffReg, Placeholder> (stream);

// Example: Matches values like a0, 0, 12(sp)
// If something remains unmatched at the end, it throws an error.
auto [reg, imm, off] = match <Reg, Imm, OffReg> (stream);
```

#### Linker

The Linker merges multiple assembly files into one.

It includes three major components:

- `SizeEstimator`: Estimates the size of each data section.
- `RelaxationPass`: Optimizes `call` instructions into `jal`.
- `Encoder`: Encodes data into binary, ensuring consistency with `SizeEstimator`.

The memory layout adheres to the RISC-V ABI standard. The `text` section starts at `0x10000`, with sections arranged as follows:

- text
- static data
  - data
  - rodata
  - bss
- heap
- stack

#### Interpreter

The Interpreter executes the binary code straightforwardly.

However, for built-in libc functions, instead of actual assembly code, these are simulated using predefined C++ code (see `include/libc/libc.h`). These functions are placed at the beginning of the `text` section, each assigned a unique address. Jumping to one of these addresses invokes the corresponding C++ function.

Additionally, to accelerate the simulation, we utilize an `icache`. This cache translates raw binary instructions into interpreter-friendly native forms during the first instruction fetch. For libc functions, the cache refers to the prewritten C++ code.

## Examples

### Extending with New Pseudo Instructions

> Refer to `src/assembly/command.cpp`

#### Reducing to Existing Instructions

Users often want to add custom pseudo instructions. The simplest way to do this is by mapping the new instruction to existing ones. For example, in our simulator, the `li rd, imm` instruction is translated into `addi rd, zero, imm`.

We can split the `li rd, imm` instruction into two parts:

1. Token `li`
2. Arguments: `rd, imm` (Register + Immediate)

Using our advanced pattern matching library, you can easily match these arguments:

```cpp
constexpr auto __insert_li = [](Assembler *ptr, TokenStream rest) {
    // Match the arguments.
    auto [rd, imm] = match <Reg, Imm> (rest);
    // Add a new storage to the current section.
    ptr->push_new <LoadImmediate> (rd, imm);
};
```

#### Adding New Instructions

For greater flexibility, you can add a new storage type to create custom pseudo instructions.

Define your instructions in `include/assembly/storage/command.h` and static storage types in `include/assembly/storage/static.h`. Don't forget to register the new types in the visitor class (`include/assembly/storage/visitor.h`), as the linker relies on this to connect instructions and static data.

For static data, ensure you implement the `real_size` and `align_size` functions in `include/linker/estimate.h`.

Note that the `visitor` is heavily used in the linker section, so you must add the appropriate rules for its derivatives.

### Extending with a New Instruction Set

TODO
