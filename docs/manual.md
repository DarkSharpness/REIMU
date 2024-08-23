# REIMU

## Quick Start

To run the simulator, you need `xmake` and `gcc-12` or higher. Install the simulator using the following commands:

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

Any unknown directives will be ignored, and incorrect arguments will cause an error.

Note that the initial value of `byte, half, word` can be a label, a number, or an expression. See the pseudo instructions section for more information.

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

| Pseudo Instruction            | Base Instruction                                                            |
| ----------------------------- | --------------------------------------------------------------------------- |
| `mv rd, rs`                   | `addi rd, rs, 0`                                                            |
| `li rd, immediate`            | `addi rd, x0, immediate`                                                    |
| `neg rd, rs`                  | `sub rd, x0, rs`                                                            |
| `not rd, rs`                  | `xori rd, rs, -1`                                                           |
| `seqz rd, rs`                 | `sltiu rd, rs, 1`                                                           |
| `snez rd, rs`                 | `sltu rd, x0, rs`                                                           |
| `sgtz rd, rs`                 | `slt rd, x0, rs`                                                            |
| `sltz rd, rs`                 | `slt rd, rs, x0`                                                            |
| `bgez rs, offset`             | `bge rs, x0, offset`                                                        |
| `blez rs, offset`             | `ble x0, rs, offset`                                                        |
| `bgtz rs, offset`             | `bgt x0, rs, offset`                                                        |
| `bltz rs, offset`             | `blt rs, x0, offset`                                                        |
| `bnez rs, offset`             | `bne rs, x0, offset`                                                        |
| `beqz rs, offset`             | `beq rs, x0, offset`                                                        |
| `bgt rs, rt, offset`          | `bgt rt, rs, offset`                                                        |
| `ble rs, rt, offset`          | `ble rt, rs, offset`                                                        |
| `bgtu rs, rt, offset`         | `bgtu rt, rs, offset`                                                       |
| `bleu rs, rt, offset`         | `bleu rt, rs, offset`                                                       |
| `j offset`                    | `jal x0, offset`                                                            |
| `jal offset`                  | `jal x1, offset`                                                            |
| `jr rs`                       | `jalr x0, 0(rs)`                                                            |
| `jalr rs`                     | `jalr x1, 0(rs)`                                                            |
| `ret`                         | `jalr x0, 0(x1)`                                                            |
| `{la, lla} rd, symbol`        | `lui rd, %hi(symbol)`, `addi rd, rd, %lo(symbol)`                          |
| `nop`                         | `addi x0, x0, 0`                                                            |
| `l{b, h, w} rd, symbol`       | `lui rd, %hi(symbol)`, `lw rd, %lo(symbol)(rd)`                            |
| `s{b, h, w} rs, symbol, rt`   | `lui rt, %hi(symbol)`, `sw rs, %lo(symbol)(rt)`                            |
| `call symbol`                 | `jal x1, symbol`                                                            |
| `tail symbol`                 | `jal x0, symbol`                                                            |

All symbols, immediate values, and offsets are treated as arithmetic expressions on labels. Currently, only addition and subtraction are supported.

For example, `main + 4 - main + %lo(main)` is valid.

Note that the `offset` or `symbol` in a branch/jump instruction represents the target destination, not the offset itself. For instance, `j main` jumps to `main`, while `j 100` jumps to address `100`.

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

TODO...
