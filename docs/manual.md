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

For more information on the debug shell, see the [debug mode](#built-in-debugger) section.

## Support

See [support](support.md) for more information.

## Built-in Debugger

See [debugger](debugger.md) for more information.

## Weight calculation

We have some weight counter to help you know the performance of your program. See [counter.h](../include/config/counter.h) for details.

Here's a list of the weight counter:

```cpp
// register_class(<group name>, <weight>, <instruction names...>)

register_class(Arith    , 1 , "add", "sub");
register_class(Upper    , 1 , "lui", "auipc");
register_class(Compare  , 1 , "slt", "sltu");
register_class(Shift    , 1 , "sll", "srl", "sra");
register_class(Bitwise  , 1 , "and", "or", "xor");
register_class(Branch   , 10, "beq", "bne", "blt", "bge", "bltu", "bgeu");
register_class(Load     , 64, "lb", "lh", "lw", "lbu", "lhu");
register_class(Store    , 64, "sb", "sh", "sw");
register_class(Multiply , 4 , "mul", "mulh", "mulhsu", "mulhu");
register_class(Divide   , 20, "div", "divu", "rem", "remu");
register_class(Jal      , 1 , "jal");
register_class(Jalr     , 2 , "jalr");

// External devices

register_class(PredictTaken, 2, "");
register_class(CacheLoad   , 4, "");
register_class(CacheStore  , 4, "");
```

## Q & A

Use [github discussions](https://github.com/DarkSharpness/REIMU/discussions/) to ask questions. Use [github issues](https://github.com/DarkSharpness/REIMU/issues/) to report bugs.
