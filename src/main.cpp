#include "utility.h"
#include "interpreter.h"
#include <iostream>
#include <algorithm>
#include <unordered_set>

int main(int argc, char** argv) {
    const auto config = dark::Config::parse(argc, argv);
    dark::Interpreter interpreter(config);
    return 0;
}

namespace dark {

auto Config::parse(int argc, char** argv) -> Config {
    static constexpr const char kInvalid[] = "Invalid command line argument: {}";

    Config config {};

    enum class CastError {
        Invalid,    //  Invalid argument
        Overflow,   //  Overflow
        Missing,    //  Missing argument
    };

    constexpr auto __to_size_t = [](std::string_view view)
        -> std::variant <std::size_t, CastError> {
        if (view.empty()) return CastError::Missing;
        std::size_t result = 0;
        for (const char c : view) {
            if (!std::isdigit(c)) return CastError::Invalid;
            result = result * 10 + (c - '0');
        }
        return result;
    };

    constexpr auto __match_prefix = [](std::string_view &view, std::initializer_list <std::string_view> list) {
        for (const auto &prefix : list)
            if (view.starts_with(prefix))
                return void(view = view.substr(prefix.size()));
        panic(kInvalid, view);
    };

    constexpr auto __match_string = [](std::string_view view, std::initializer_list <std::string_view> list) {
        for (const auto &prefix : list)
            if (view == prefix) return;
        panic(kInvalid, view);
    };

    constexpr auto __help = [](std::string_view view) {
        std::cout <<
R"(This is a RISC-V simulator. Usage: simulator [options]
Options:
  -h, --help                                Display help information.
  -v, --version                             Display version information.

  -en-<option>, -enable-<option>            Enable a option.
  -dis-<option>, -disable-<option>          Disable a option.
                                            * Available options: detail, debug, cache

  -w<name>=<value>, -weight-<name>=<value>  Set weight (cycles) for a specific assembly command.
                                            The name can be either a opcode name or a group name.
                                            Example: -wadd=1 -wmul=3 -wmemory=100 -wbranch=3

  -t=<time>, -time=<time>                   Set maximum time (cycles) for the simulator, default unlimited.

  -m=<mem>, -mem=<mem>, -memory=<mem>       Set memory size (bytes) for the simulator, default 256MB.
                                            We support K/M suffix for kilobytes/megabytes.
                                            Note that the memory has a hard limit of 1GB.
                                            Example: -m=114K -m=514M -mem=1919 -memory=810
  -i=<file>, -input=<file>                  Set input file for the simulator.
  -o=<file>, -output=<file>                 Set output file for the simulator.
  -f=<file>,... -file=<file>,...            Set input files as the assembly code.
)";
        std::exit(EXIT_SUCCESS);
    };

    constexpr auto __version = [](std::string_view view) {
        std::cout << "Version: 0.1.0\n";
        std::exit(EXIT_SUCCESS);
    };

    const auto __set_option = [&](std::string_view view, bool enable) {
        panic_if(view.empty(), "Invalid option: {}", view);
        auto &table = config.option_table;
        panic_if(table.try_emplace(view, enable).second == false, "Duplicate option: {}", view);
    };

    const auto __set_weight = [&](std::string_view view) {
        static constexpr const char kInvalid[]      = "Invalid weight argument: {}";
        static constexpr const char kMissing[]      = "Missing weight: {}";
        static constexpr const char kDuplicate[]    = "Duplicate weight: {}";
        static constexpr const char kNonNegative[]  = "Weight must be non-negative integer: {}";
        static constexpr const char kOverflow[]     = "Weight overflow: {} \n"
                                                      "  Hint: Maximum Weight = {}";
        static constexpr std::size_t kThreshold     = 1919810;

        std::size_t next = view.find_first_of('=');
        panic_if(next == std::string_view::npos, kInvalid, view);

        std::string_view name = view.substr(0, next);
        auto weight = __to_size_t(view.substr(next + 1));

        if (std::holds_alternative <CastError> (weight)) {
            switch (std::get <CastError> (weight)) {
                case CastError::Invalid:
                    panic(kNonNegative, view);
                case CastError::Overflow:
                    panic(kOverflow, view, kThreshold);
                case CastError::Missing:
                    panic(kMissing, name);
                default:
                    runtime_assert(false);
            }
        } else {
            auto &table = config.weight_table;
            auto  value = std::get <std::size_t> (weight);
            panic_if(table.try_emplace(name, value).second == false, kDuplicate, name);
        }
    };

    const auto __set_timeout = [&](std::string_view view) {
        static constexpr const char kInvalid[]      = "Invalid timeout argument: {}";
        static constexpr const char kMissing[]      = "Missing timeout: {}";
        static constexpr const char kDuplicate[]    = "Duplicate timeout: {}";
        static constexpr const char kNonNegative[]  = "Timeout must be non-negative integer: {}";
        static constexpr const char kOverflow[]     = "Timeout overflow: {} \n"
                                                      "  Hint: Maximum Timeout = {}";
        static constexpr std::size_t kThreshold     = 1ull << 62;

        auto timeout = __to_size_t(view);
        if (std::holds_alternative <CastError> (timeout)) {
            switch (std::get <CastError> (timeout)) {
                case CastError::Invalid:
                    panic(kNonNegative, view);
                case CastError::Overflow:
                    panic(kOverflow, view, kThreshold);
                case CastError::Missing:
                    panic(kMissing, view);
                default:
                    runtime_assert(false);
            }
        } else {
            panic_if(config.maximum_time != config.uninitialized, kDuplicate, view);
            config.maximum_time = std::get <std::size_t> (timeout);
        }
    };

    const auto __set_memory = [&](std::string_view view) {
        static constexpr const char kInvalid[]      = "Invalid memory argument: {}";
        static constexpr const char kMissing[]      = "Missing memory: {}";
        static constexpr const char kDuplicate[]    = "Duplicate memory: {}";
        static constexpr const char kNonNegative[]  = "Memory must be non-negative integer: {}";
        static constexpr const char kOverflow[]     = "Memory overflow: {} \n"
                                                      "  Hint: Maximum Memory = {}";
        static constexpr std::size_t kThreshold     = 1 << 30;  // 1GB

        std::size_t factor = 1;
        if (view.size() && view.back() == 'K') {
            factor = 1 << 10;
            view.remove_suffix(1);
        } else if (view.size() && view.back() == 'M') {
            factor = 1 << 20;
            view.remove_suffix(1);
        }

        auto memory = __to_size_t(view);
        if (std::holds_alternative <CastError> (memory)) {
            switch (std::get <CastError> (memory)) {
                case CastError::Invalid:
                    panic(kNonNegative, view);
                case CastError::Overflow:
                    panic(kOverflow, view, kThreshold);
                case CastError::Missing:
                    panic(kMissing, view);
                default:
                    runtime_assert(false);
            }
        } else {
            panic_if(config.storage_size != config.uninitialized, kDuplicate, view);
            config.storage_size = std::get <std::size_t> (memory) * factor;
        }
    };

    const auto __set_assembly = [&](std::string_view view) {
        panic_if(view.empty(), "Invalid input file: {}", view);
        auto &files = config.assembly_files;
        panic_if(!files.empty(), "Duplicate input file: {}", view);

        std::size_t next = view.find_first_of(',');
        while (next != std::string_view::npos) {
            files.emplace_back(view.substr(0, next));
            view.remove_prefix(next + 1);
            next = view.find_first_of(',');
        }
        files.emplace_back(view);

        std::ranges::sort(config.assembly_files);
        std::size_t rest = std::ranges::unique(files).size();
        files.resize(files.size() - rest);
    };

    for (int i = 1 ; i < argc ; ++i) {
        std::string_view view = argv[i];
        if (view.size() < 2 || view.front() != '-') {
            panic(kInvalid, view);
        } else switch (view[1]) {
            case 'h':   __match_string(view, {"-help", "-h"});
                        __help(view);     break;
            case 'v':   __match_string(view, {"-version", "-v"});
                        __version(view);  break;
            case 'e':   __match_prefix(view, {"-enable-", "-en-"});
                        __set_option(view, true); break;
            case 'd':   __match_prefix(view, {"-disable-", "-dis-"});
                        __set_option(view, false); break;
            case 'w':   __match_prefix(view, {"-weight-", "-w"});
                        __set_weight(view); break;
            case 't':   __match_prefix(view, {"-time=", "-t="});
                        __set_timeout(view); break;
            case 'm':   __match_prefix(view, {"-memory=", "-mem=", "-m="});
                        __set_memory(view); break;
            case 'i':   __match_prefix(view, {"-input=", "-i="});
                        panic_if(!config.input_file.empty(), "Duplicate input file: {}", view);
                        config.input_file = view; break;
            case 'o':   __match_prefix(view, {"-output=", "-o="});
                        panic_if(!config.output_file.empty(), "Duplicate output file: {}", view);
                        config.output_file = view; break;
            case 'f':   __match_prefix(view, {"-file=", "-f="});
                        __set_assembly(view); break;

            default:    panic(kInvalid, view);
        }
    }

    config.initialize_with_check();
    if (config.option_table["detail"])
        config.print_in_detail();

    return config;
}

} // namespace dark
