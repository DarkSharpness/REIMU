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

### Instructions

We support RV32I base instruction set and RV32M standard extension are supported except `fence`, `fence.i`, `ecall`, `ebreak`, and `csr` instructions.

### Pseudo Instructions

We support the following pseudo instructions.

| Pseudo instructions       | Base instructions         |
| -------------------       | -----------------         |
| `mv rd, rs`               | `addi rd, rs, 0`          |
| `li rd, immediate`        | `addi rd, x0, immediate`  |
| `neg rd, rs`              | `sub rd, x0, rs`          |
| `not rd, rs`              | `xori rd, rs, -1`         |
| `seqz rd, rs`             | `sltiu rd, rs, 1`         |
| `snez rd, rs`             | `sltu rd, x0, rs`         |
| `sgtz rd, rs`             | `slt rd, x0, rs`          |
| `sltz rd, rs`             | `slt rd, rs, x0`          |
| `bgez rs, offset`         | `bge rs, x0, offset`      |
| `blez rs, offset`         | `ble x0, rs, offset`      |
| `bgtz rs, offset`         | `bgt x0, rs, offset`      |
| `bltz rs, offset`         | `blt rs, x0, offset`      |
| `bnez rs, offset`         | `bne rs, x0, offset`      |
| `beqz rs, offset`         | `beq rs, x0, offset`      |
| `bgt rs, rt, offset`      | `bgt rt, rs, offset`      |
| `ble rs, rt, offset`      | `ble rt, rs, offset`      |
| `bgtu rs, rt, offset`     | `bgtu rt, rs, offset`     |
| `bleu rs, rt, offset`     | `bleu rt, rs, offset`     |
| `j offset`                | `jal x0, offset`          |
| `jal offset`              | `jal x1, offset`          |
| `jr rs`                   | `jalr x0, 0(rs)`          |
| `jalr rs`                 | `jalr x1, 0(rs)`          |
| `ret`                     | `jalr x0, 0(x1)`          |
| `{la, lla} rd, symbol`    | `lui rd, %hi(symbol)`, `addi rd, rd, %lo(symbol)` |
| `nop`                     | `addi x0, x0, 0`          |
| `l{b, h, w} rd, symbol`   | `lui rd, %hi(symbol)`, `lw rd, %lo(symbol)(rd)` |
| `s{b, h, w} rs, symbol, rt` | `lui rt, %hi(symbol)`, `sw rs, %lo(symbol)(rt)` |
| `call symbol`             | `jal x1, symbol`          |
| `tail symbol`             | `jal x0, symbol`          |

Note that all the symbol, immediate and offset values are treated as normal immediate values, which can be arithmetic expressions on labels.

Example: `main + 4 - main + main`. We support only add and sub currently.

Note that the `offset` or `symbol` destination is actually the destination of the branch/jump instruction, not the offset. In other words, `j main` will jump to `main`, while `j 100` will jump to the address `100`.

### Libc Functions

We support the following libc functions:

```cpp
register_functions(
    puts, putchar, printf, sprintf, getchar, scanf, sscanf, // IO functions
    malloc, calloc, realloc, free, // Memory management
    memset, memcmp, memcpy, memmove, // Memory manipulation
    strcpy, strlen, strcat, strcmp // Strings manipulation
);
```

It works just as what you expect, except that `printf` and `sprintf` only support passing argument in register, and only the simplest format string is supported.
