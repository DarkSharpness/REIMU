# Debugger

The simulator includes a built-in debug shell to assist in program debugging. To enable debug mode, use the `--debug` flag when running the simulator. It is highly recommended to use the `-i` and `-o` flags to specify input and output files, to avoid interference with the debug shell.

```shell
reimu --debug -i test.in -o test.out
```

## Boundary checks

The simulator already enforces strict boundary checks on memory accesses. Any misaligned or out-of-bound memory access will result in simulation termination rather than causing the simulator to crash, as might happen with segmentation faults in real hardware.

In debugger mode, we impose even stricter checks on calling conventions. When you call a function, you must return to the correct address. Failure to do so will cause the simulator to terminate with an error message. Additionally, you must ensure that the stack pointer is correctly restored to its original value before returning from a function.

```assembly
    .text

# In this function, the stack pointer (sp) is not correctly restored before returning.
# In non-debug mode, the program might still run without obvious issues, but in debug mode,
# it will terminate with an error message after the function returns, indicating stack corruption.
    .globl wrong_0
wrong_0:
    addi sp, sp, -16
    ret

# The debugger only recognizes standard return instructions, specifically `ret` (which is `jalr zero, 0(ra)`).
# Although the logic of the following code is correct, our debugger will flag it and
# terminate the program with an error message.
    .globl wrong_1
wrong_1:
    mv t0, sp
    jalr t0
```

## Supported Commands

The debug shell supports the following commands:

| Command               | Full Name     | Description                                                               |
| --------------------- | ------------- | ------------------------------------------------------------------------- |
| `s n`                 | step          | Execute the next `n` instructions                                         |
| `c`                   | continue      | Continue execution until the next stop point or the end of the program    |
| `b addr`              | breakpoint    | Set a breakpoint at address `addr`                                        |
| `d n`                 | delete        | Delete the breakpoint with the specified number `n`                       |
| `x mode addr`         | -             | Display memory content at address `addr` in the specified `mode`          |
| `p mode expr`         | print         | Print the value of the expression `expr` in the specified `mode`          |
| `bt`                  | backtrace     | Display the backtrace of the current function call stack                  |
| `i mode`              | info          | Display information about the current state of the simulator              |
| `q`                   | quit          | Exit the debug shell; the simulator continues running until the end       |
| `h n`                 | history       | Display the last `n` executed instructions                                |
| `display mode expr`   | display       | Continuously display the value of the expression `expr` in the specified `mode` |
| `undisplay n`         | undisplay     | Stop displaying the value of the expression with the specified number `n` |
| `w mode expr`         | watch         | Watch the value of the expression `expr` in the specified `mode`          |
| `unwatch n`           | unwatch       | Delete the watchpoint with the specified number `n`                       |
| `help`                | help          | Display the help message                                                   |

## Expressions

Expressions can include registers, immediate values, and labels. Arithmetic expressions involving these elements are supported.

- Registers can be referenced as `$name` or `x$which`, e.g., `$ra`, `$x1`. The special register `$pc` represents the program counter.
- Labels are represented by their names, e.g., `main`.
- Immediate values can be in hexadecimal (`0x10`), decimal (`16`), or binary (`0b10000`) format.

## Modes

### Modes for `p` Command

- `x` for hexadecimal
- `d` for decimal
- `c` for character
- `t` for binary
- `i` for instruction
- `f` for floating-point
- `a` for address

```shell
p x $pc  # Print the value of the program counter in hexadecimal
p a $pc  # Print the address of the program counter in a formatted manner
p d $pc  # Print the value of the program counter in decimal
```

### Modes for `x` Command

- `b` for byte
- `h` for half-word
- `w` for word

By default, the number of units is 1.

```shell
x 10i $pc   # Display the next 10 instructions  [pc, pc + 40)
x 10w $sp   # Display the next 10 words starting from [sp, sp + 40)
x b num     # Display 1 byte at the symbol `num`
```

### Modes for `i` Command

- `breakpoint`: List all breakpoints
- `symbol`: List all symbols
- `shell`: Show the command history of the debug shell
- `display`: List all display expressions
- `watchpoint`: List all watchpoints

```shell
info breakpoint
```

### Modes for `display` Command

- First, specify whether the expression is a memory address (`m`) or a value (`v`).
- For memory addresses (`m`), use modes identical to the `x` command.
- For values (`v`), use modes identical to the `p` command.

```shell
display m 2w $sp    # Display the 2 words at [sp, sp + 8) each time
display m 10i $pc   # Display the next 10 instructions each time
display v a $pc     # Display the value of the program counter in hexadecimal each time
display v x $a0 + 8 # Display the value of a0 + 8 in hexadecimal each time
```

### Modes for `w` Command

- Specify whether to watch a memory address (`m`) or a register (`v`).
- For memory addresses (`m`), use modes similar to the `x` command (excluding instruction mode and without specifying a number).

```shell
watch m w $sp - 4  # Watch the word at stack pointer minus 4
watch v $a0        # Watch the value of register a0
```

**Note:** Memory watchpoints do not automatically update their address. For instance, if you set a watchpoint on `$sp - 4` while `$sp` is `0x100`, it will continue watching `[0xfc, 0x100)` even if `$sp` changes.

## Useful Examples and Tips

```shell
x 10i $pc   # Display the next 10 instructions
x 10w $sp   # Display the next 10 words starting from the stack pointer
bt          # Show the backtrace of the current function call stack
s 100       # Execute the next 100 instructions
b main + 8  # Set a breakpoint at the address of main + 8
w m w .num  # Watch the word at the address of the symbol `num`
w r w $s11  # Watch the value of register s11
c           # Continue execution until the next stop point or the end of the program
```

To maximize the power of the debug shell, consider marking all functions as global. This allows the debugger to trace symbol information and provide more accurate hints.
