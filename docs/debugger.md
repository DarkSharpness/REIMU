# Debugger

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

## Expressions

Expressions can include registers, immediate values, and labels. Arithmetic expressions on these elements are supported.

Registers can be represented by `$name` or `x$which`, e.g., `$ra`, `$x1`.

Labels are represented by their names, e.g., `main`.

## Modes

### Modes for `p` command:

- `x` for hexadecimal
- `d` for decimal
- `c` for character
- `t` for binary
- `i` for instruction
- `f` for floating point
- `a` for address

### Modes for `x` command (number + suffix), suffixes:

- `b` for byte
- `h` for half word
- `w` for word

The default number is $1$.

```shell
x 10i $pc
```

### Modes for `i` command:

- `breakpoint`: Print all breakpoints
- `symbol`: Print all symbols

## Examples

```shell
x 10i $pc   # Print the next 10 instructions
x 10w $sp   # Print the next 10 words starting from the stack pointer
bt          # Print the backtrace of the current function call stack
s 10        # Execute the next 10 instructions
b main + 8  # Set a breakpoint at the address of main + 8
c           # Continue execution until the next breakpoint or the end of the program
x 2i $pc - 8 # Print the previous 2 instructions
```
