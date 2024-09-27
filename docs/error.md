# Error message

## Configuration error

When invalid command line arguments are passed to the program, the program will print an error message and exit.

```text
Fatal error: Fail to parse command line argument.
  <Cause>
```

## Assemble/Link time error

When assembling the given source code and some error is detected, the program will print an error message and exit.

```text
Fatal error: Fail to parse/link source assembly.
  <Cause>
Failure at <file>:<line>
<line - 1>  | <source code>
<line>      | <source code>
<line + 1>  | <source code>
```

File name and line number are printed to help you locate the error in the source code.

## Runtime error

When the interpreter is running and detects an error, the program will print an error message and exit.

```text
Fatal error: Fail to execute the program.
  <Cause>
```

If you want to further debug your program, you may pass `--debug` option to the simulator. For detailed information, please refer to the [debugging](debugger.md).

### Common runtime errors

| Error message     | Description   |
| ---               | ---           |
| ... misaligned    | Memory access is not aligned. e.g. `lw t0, 1(t1)` requires `t1` to be aligned to 4 bytes. |
| ... out of bound  | Memory access is out of bound. |
| unknown instruction | Unknown instruction. |
| Division by zero  | Division or modulo by zero. |
| Out of memory     | Heap memory is exhausted. |
| libc::<name>: ... | Invalid argument passed to libc function. |

## Other errors

In any case, the simulator should not crash. If you got errors like "segmentation fault" or "bus error", please report it as a bug.

If you got "Internal error", that's also a bug. Please report it with the error message.
