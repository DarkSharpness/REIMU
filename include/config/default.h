#pragma once
#include "declarations.h"
#include <string_view>

namespace dark::config {

static constexpr std::size_t kInitMemorySize   = 256 * 1024 * 1024;
static constexpr std::size_t kInitStackSize    = 32 * 1024;
static constexpr std::size_t kInitTimeOut      = static_cast<std::size_t>(-1);
static constexpr std::string_view kStdin       = "<stdin>";
static constexpr std::string_view kStdout      = "<stdout>";
static constexpr std::string_view kStderr      = "<stderr>";
static constexpr std::string_view kInitStdin   = "<stdin>";
static constexpr std::string_view kInitStdout  = "<stdout>";
static constexpr std::string_view kInitProfile = "<stderr>";
static constexpr std::string_view kInitAnswer  = "test.ans";

// clang-format off

static constexpr std::string_view kSupportedOptions[] = {
    "--silent",
    "--detail",
    "--debug",
    "--cache",
    "--predictor",
    "--all",
    "--oj-mode",
};

static constexpr std::initializer_list <std::string_view> kInitAssemblyFiles = {
    "builtin.s",
    "test.s",
};

static constexpr std::string_view kHelpMessage = 
R"(This is a RISC-V simulator. Usage: reimu [options]
Options:
  -h, --help                        Display help information.
  -v, --version                     Display version information.
  --silent                          Disable all output except for
                                      1. program output
                                      2. fatal error
  --detail                          Print the configuration details.
                                    Conflicts with --silent.
  --debug                           Use built-in gdb.
  --cache                           Enable cache simulation.
  --predictor                       Enable branch predictor simulation.
  --all                             Enable all optimizations.
                                    Equivalent to --cache --predictor.
  --oj-mode                         Settings for the online judge.

Configurations:
  --<option>                        Enable a specific option (see above).

  -w<name>=<value>,
  --weight-<name>=<value>           Set weight (cycles) for a specific assembly command.
                                    The name can be either an opcode name or a group name.
                                    - Example: -wload=100 -wbranch=3

  -t=<time>, --time=<time>          Set maximum instructions for the simulator.
                                    Note that this time is measured by instructions, not cycles.

  -m=<mem>, --memory=<mem>          Set memory size (bytes) for the simulator, default 256MB.
                                    We support K/M suffix for kilobytes/megabytes.
                                    - Example: -m=114K --memory=810

  -s=<mem>, --stack=<mem>           Set stack size (bytes) for the simulator, default 32KB.
                                    We support K/M suffix for kilobytes/megabytes.

  -i=<file>, --input=<file>         Set input file for the simulator, default <stdin>.
                                    If <file> = <stdin>, the simulator will read from stdin.
                                    - Example: -i=test.in, -i="<stdin>"

  -o=<file>, --output=<file>        Set output file for the simulator, default <stdout>.
                                    If <file> = <stdout>, the simulator will write to stdout.
                                    If <file> = <stderr>, the simulator will write to stderr.
                                    - Example: -o=test.out, -o="<stdout>", -o="<stderr>"

  -f=<file>,... --file=<file>,...   Set input assembly files for the simulator.
                                    The simulator will decode and link these files in order.
                                    - Example: -f=test.s,builtin.s

  -p=<file>, --profile=<file>       Set the profiling output for the simulator, default <stderr>.
                                    If <file> = <stdout>, the simulator will write to stdout.
                                    If <file> = <stderr>, the simulator will write to stderr.
                                    - Example: -p=prof.txt, -p="<stdout>"

  -a=<file>, --answer=<file>        Set the answer file for the simulator, default test.ans.
                                    This option can only work with the oj-mode.
)";

// clang-format on

static constexpr std::string_view kVersionMessage = "reimu 1.0.0\n";

} // namespace dark::config
