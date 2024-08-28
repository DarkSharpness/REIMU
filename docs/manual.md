# REIMU

## Quickest start

> TL;DR: If you are lazy, just use the shell code in script folder.

```shell
# For WSL 2/Ubuntu
./script/install_wsl.sh
# For Arch Linux
./script/install_arch.sh
```

## Quick Start

### Installation

To run the simulator, you need `xmake` and `gcc-12` (including `g++-12`) or higher. If you don't have them installed, refer to [installation](installation.md) for more details. Then, install the simulator using the following commands:

```shell
git clone git@github.com:DarkSharpness/REIMU.git
cd REIMU
xmake f -p linux -a x86_64 -m release
xmake
xmake install --admin
# You may need to enter your password to install the simulator
```

After installation, run the simulator with:

```shell
reimu
```

When you want to update the simulator, just run the following commands:

```shell
git pull
xmake
xmake install --admin
```

### Running a Program

The simulator reads command-line arguments and executes the program with the specified configuration.

By default, the input assembly files are `test.s` and `builtin.s` in the current directory. The program's input defaults to stdin and output to stdout. You can change the input and output files using the -i and -o flags, respectively:

You can pass the arguments in the form `-<option>=<value>` or `-<option> <value>`

```shell
reimu -i=test.in -o test.out
```

For detailed information on all available command-line arguments, use the --help flag (recommended with less):

```shell
reimu --help | less
```

If you need to debug your program, you can use the `--debug` flag to enable the built-in debug shell:

```shell
reimu --debug -i test.in -o test.out
```

For more information on the debug shell, see the [debug mode](#built-in-debug-mode) section.

## Support

### Directives

The following directives are supported:

| Directive                     | Description                                                                 |
| ----------------------------- | --------------------------------------------------------------------------- |
| `{.data, .sdata}`             | Data segment                                                                |
| `.text`                       | Text segment                                                                |
| `.rodata`                     | Read-only data segment                                                      |
| `{.bss, .sbss}`               | Uninitialized data segment                                                  |
| `{.align, .p2align} n`        | Align the next data to `1 << n` bytes                                       |
| `.balign n`                   | Align the next data to `n` bytes (must be a power of 2)                     |
| `.globl symbol`               | Declare a global symbol                                                     |
| `.section name`               | Switch to a different section                                               |
| `.zero n`                     | Allocate `n` zero bytes                                                     |
| `.byte a,b,...`               | Initialize bytes to `a`, `b`, ...                                           |
| `.half a,b,...`               | Initialize half-words to `a`, `b`, ... (must be 2-byte aligned)             |
| `.word a,b,...`               | Initialize words to `a`, `b`, ... (must be 4-byte aligned)                  |
| `{.asciz, .string} "string"`  | Null-terminated string                                                      |

Any unknown directives will be ignored, and incorrect arguments will lead to an error.

Note that the initial value of `byte, half, word` can be a label, a number, or an expression. See the pseudo instructions section for more information. Example: `.word main, 0x1234, %lo(main) + 4`

### Labels

Sadly, local labels are not supported yet. A valid label character is defined as:

```cpp
// [a-zA-Z0-9_.@$]
/* Whether the character is a valid token character. */
bool is_label_char(char c) {
    return std::isalnum(c) || c == '_' || c == '.' || c == '@' || c == '$';
}
```

Note that `.` alone is a special label that represents the current address.

### Relocations

The simulator supports the following relocations:

- `%lo(symbol)`: The lower 12 bits of the symbol's address
- `%hi(symbol)`: The higher 20 bits of the symbol's address
- `%pcrel_lo(symbol)`: The lower 12 bits of the symbol's address relative to the current position
- `%pcrel_hi(symbol)`: The higher 20 bits of the symbol's address relative to the current position

### Instructions

The simulator supports the RV32I base instruction set and RV32M standard extension, with the exception of `fence`, `fence.i`, `ecall`, `ebreak`, and `csr` instructions.

### Pseudo Instructions

The following pseudo instructions are supported:

| Pseudo Instruction            | Base Instruction                                  | Meaning                                      |
| ----------------------------- | ------------------------------------------------- | --------------------------------------------- |
| `mv rd, rs`                   | `addi rd, rs, 0`                                  | Set `rd` to the value of `rs`                 |
| `li rd, imm`                  | `lui rd, %hi(imm)`, `addi rd, rd, %lo(imm)`       | Load immediate value into `rd`                |
| `neg rd, rs`                  | `sub rd, x0, rs`                                  | Set `rd` to the negation of `rs`              |
| `not rd, rs`                  | `xori rd, rs, -1`                                 | Set `rd` to the bitwise NOT of `rs`           |
| `seqz rd, rs`                 | `sltiu rd, rs, 1`                                 | Set if `rs` = `zero`                         |
| `snez rd, rs`                 | `sltu rd, x0, rs`                                 | Set if `rs` != `zero`                        |
| `sgtz rd, rs`                 | `slt rd, x0, rs`                                  | Set if `rs` > `zero`                         |
| `sltz rd, rs`                 | `slt rd, rs, x0`                                  | Set if `rs` < `zero`                         |
| `bgez rs, offset`             | `bge rs, x0, offset`                              | Branch if `rs` >= `zero`                     |
| `blez rs, offset`             | `ble x0, rs, offset`                              | Branch if `rs` <= `zero`                     |
| `bgtz rs, offset`             | `bgt x0, rs, offset`                              | Branch if `rs` > `zero`                      |
| `bltz rs, offset`             | `blt rs, x0, offset`                              | Branch if `rs` < `zero`                      |
| `bnez rs, offset`             | `bne rs, x0, offset`                              | Branch if `rs` != `zero`                     |
| `beqz rs, offset`             | `beq rs, x0, offset`                              | Branch if `rs` = `zero`                      |
| `bgt rs, rt, offset`          | `bgt rt, rs, offset`                              | Branch if `rs` > `rt`                        |
| `ble rs, rt, offset`          | `ble rt, rs, offset`                              | Branch if `rs` <= `rt`                       |
| `bgtu rs, rt, offset`         | `bgtu rt, rs, offset`                             | Branch if `rs` > `rt` (unsigned)             |
| `bleu rs, rt, offset`         | `bleu rt, rs, offset`                             | Branch if `rs` <= `rt` (unsigned)            |
| `j offset`                    | `jal x0, offset`                                  | Jump to `offset`                             |
| `jal offset`                  | `jal x1, offset`                                  | Jump to `offset` and set `ra` to the next PC |
| `jr rs`                       | `jalr x0, 0(rs)`                                  | Jump to `rs`                                 |
| `jalr rs`                     | `jalr x1, 0(rs)`                                  | Jump to `rs` and set `ra` to the next PC     |
| `ret`                         | `jalr x0, 0(x1)`                                  | Return to the address in `ra`                |
| `{la, lla} rd, symbol`        | `lui rd, %hi(symbol)`, `addi rd, rd, %lo(symbol)` | Load the address of `symbol` into `rd`       |
| `nop`                         | `addi x0, x0, 0`                                  | No operation                                 |
| `l{b, h, w} rd, symbol`       | `lui rd, %hi(symbol)`, `lw rd, %lo(symbol)(rd)`   | Load a byte, half word, or word from `symbol`|
| `s{b, h, w} rs, symbol, rt`   | `lui rt, %hi(symbol)`, `sw rs, %lo(symbol)(rt)`   | Store a byte, half word, or word to `symbol` |
| `call symbol`                 | `jal x1, symbol` or `auipc x1, %pcrel_hi(symbol)`, `jalr x1, %pcrel_lo(symbol)(x1)` | Call function at `symbol` |
| `tail symbol`                 | `jal x0, symbol` or `auipc x6, %pcrel_hi(symbol)`, `jalr x1, %pcrel_lo(symbol)(x6)` | Tail call at `symbol` |

All symbols, immediate values, and offsets are treated as arithmetic expressions on labels. Currently, only addition and subtraction are supported.

For example, `main + 4 - main + %lo(main)` is valid.

Note that the `offset` or `symbol` in a branch/jump instruction represents the target destination, not the offset itself. For instance, `j main` jumps to `main`, while `j 100` jumps to address `100`. If you want to jump with offset, use `. + offset` instead. Example:

```assembly
j . + 8
nop # This instruction will be skipped
...
```

In addition, we have some optimizations for `li` and `call` commands. If the immediate value for `li` is some literal constant within the range of 12 bits (`[-2048, 2047]`), we will use `addi` instead of `lui` and `addi`. If the immediate value for `li` is times of 4096, we will use `lui` alone to load the value. For `call` command, we will use `jal` directly if the symbol is within the range of 20 bits, which is often the case since the program is mostly not that large.

```assembly
li x1, 8192 # will be optimized to lui x1, 2
li x2, 1234 # will be optimized to addi x2, x0, 1234
li x2, 9999 # cannot be optimized, will be lui x2, 2 and addi x2, x2, 1807
call main   # might be optimized to jal x1, main
```

### Libc Functions

The following libc functions are supported:

```cpp
register_functions(
    puts, putchar, printf, sprintf, getchar, scanf, sscanf, // I/O functions
    malloc, calloc, realloc, free, // Memory management
    memset, memcmp, memcpy, memmove, // Memory manipulation
    strcpy, strlen, strcat, strcmp // String manipulation
);
```

These functions behave as expected, with the exception that `printf` and `sprintf` only support arguments passed in registers, and only the simplest format strings are supported.

Notable features:

- `malloc` returns pointers aligned to 16 bytes.
- `free` performs nothing currently.

### Built-in Debug Mode

The simulator includes a simple built-in debug shell to help you debug your programs. To enable debug mode, run the simulator with the `--debug` flag. It is recommended to use the `-i` and `-o` flags to specify input and output files, preventing interference with the debug shell.

```shell
reimu --debug -i test.in -o test.out
```

The debug shell supports the following commands:

| Command       | Full Name   | Description                                                             |
| ------------- | ----------- | ----------------------------------------------------------------------- |
| `p mode expr` | print       | Print the value of the expression `expr` in the specified `mode`        |
| `x mode addr` | -           | Print the memory content at address `addr` in the specified `mode`      |
| `b addr`      | breakpoint  | Set a breakpoint at address `addr`                                      |
| `d n`         | delete      | Delete the breakpoint with the specified number `n`                     |
| `c`           | continue    | Continue execution until the next breakpoint or the end of the program  |
| `q`           | quit        | Exit the debug shell; the simulator will continue to run until the end  |
| `i mode`      | info        | Display information about the current state of the simulator            |
| `bt`          | backtrace   | Print the backtrace of the current function call stack                  |
| `s n`         | step        | Execute the next `n` instructions                                       |
| `h n`         | history     | Print the last `n` executed instructions                                |

#### Expressions

Expressions can include registers, immediate values, and labels. Arithmetic expressions on these elements are supported.

Registers can be represented by `$name` or `x$which`, e.g., `$ra`, `$x1`.

Labels are represented by their names, e.g., `main`.

#### Modes

Modes for `p` command:

- `x` for hexadecimal
- `d` for decimal
- `c` for character
- `t` for binary
- `i` for instruction
- `f` for floating point
- `a` for address

Modes for `x` command (number + suffix), suffixes:

- `b` for byte
- `h` for half word
- `w` for word

The default number is $1$.

Modes for `i` command:

- `breakpoint`: Print all breakpoints
- `symbol`: Print all symbols

#### Examples

```shell
x 10i $pc   # Print the next 10 instructions
x 10w $sp   # Print the next 10 words starting from the stack pointer
bt          # Print the backtrace of the current function call stack
s 10        # Execute the next 10 instructions
b main + 8  # Set a breakpoint at the address of main + 8
c           # Continue execution until the next breakpoint or the end of the program
x 2i $pc - 8 # Print the previous 2 instructions
```

## Q & A

Use [github discussions](https://github.com/DarkSharpness/REIMU/discussions/) to ask questions. Use [github issues](https://github.com/DarkSharpness/REIMU/issues/) to report bugs.
