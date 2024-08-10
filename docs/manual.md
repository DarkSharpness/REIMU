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
