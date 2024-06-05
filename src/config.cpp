#include <utility.h>
#include <config.h>
#include <default_config.h>
#include <ranges>
#include <algorithm>

namespace dark {

using __config::kWeightCount;
using __config::weight_ranges;
using __config::kOther;


using _Option_Map_t = decltype(Config::option_table);
using _Weight_Map_t = decltype(Config::weight_table);

static constexpr std::array option_list = []() {
    using _Pair_t = std::pair <std::string_view, bool>;
    return std::array {
        _Pair_t("debug",    false),
        _Pair_t("cache",    false),
        _Pair_t("detail",   false),
    };
} ();

static constexpr std::array weight_list = []() {
    using _Pair_t = std::pair <std::string_view, std::size_t>;
    std::array <_Pair_t, kWeightCount> table {};
    std::size_t which {};
    for (const auto &[name, list, weight] : weight_ranges) {
        if (weight != kOther) {
            for (auto item : list)
                table[which++] = { item, weight };
        } else {
            for (auto item : list) {
                auto pos = item.find('=');
                std::size_t weight = 0;
                for (auto c : item.substr(pos + 1)) {
                    if (c < '0' || c > '9') throw;
                    weight = weight * 10 + c - '0';
                }
                item = item.substr(0, pos);
                table[which++] = { item, weight };
            }
        }
    }
    if (which != kWeightCount) throw;
    return table;
} ();

static void check_option(_Option_Map_t &table) {
    _Option_Map_t default_options { std::begin(option_list), std::end(option_list) };
    for (const auto &[key, value] : table) {
        panic_if(default_options.count(key) == 0, "Unknown option: {}", key);
    }
    for (const auto &[key, value] : default_options) {
        table.try_emplace(key, value);
    }
}

static auto find_weight_range(std::string_view need)
-> std::optional <std::span <const std::string_view>> {
    for (const auto &[name, list, _] : weight_ranges)
        if (name == need) return list;
    return std::nullopt;
}

static void check_weight(_Weight_Map_t &table) {
    _Weight_Map_t default_weights { std::begin(weight_list), std::end(weight_list) };

    for (const auto &[key, value] : table) {
        if (default_weights.count(key) != 0) continue; 

        auto list = find_weight_range(key);
        panic_if(!list.has_value(), "Unknown weight range: {}", key);

        for (const auto &item : *list) {
            default_weights[item] = value;
        }
    }

    for (const auto &[key, value] : default_weights) {
        table.try_emplace(key, value);
    }
}

void Config::initialize_with_check() {
    if (this->input_file.empty())
        this->input_file = "test.in";
    if (this->output_file.empty())
        this->output_file = "test.out";
    if (this->assembly_files.empty())
        this->assembly_files = { "test.s", "builtin.s" };

    if (storage_size == Config::uninitialized)
        storage_size = 256 * 1024 * 1024;

    // Will not time out by default
    if (maximum_time == Config::uninitialized)
        maximum_time = Config::uninitialized;

    check_option(this->option_table);
    check_weight(this->weight_table);
}

void Config::print_in_detail() const {
    std::cout << std::format("{:=^80}\n", " Configuration details ");

    std::cout << std::format("  Input file: {}\n", this->input_file);
    std::cout << std::format("  Output file: {}\n", this->output_file);
    std::cout << std::format("  Assembly files: ");
    for (const auto& file : this->assembly_files)
        std::cout << file << ' ';

    std::cout << std::format("\n  Storage size: {} bytes ({:.2f} MB)\n",
        this->storage_size, double(this->storage_size) / (1024 * 1024));

    if (this->maximum_time == Config::uninitialized) {
        std::cout << "  Maximum time: no limit\n";
    } else {
        std::cout << std::format("  Maximum time: {} cycles\n", this->maximum_time);
    }

    // Format string for printing options and weights
    static constexpr char kFormat[] = "    - {:<8} = {}\n";

    std::cout << "  Options:\n";
    for (const auto& [key, value] : this->option_table)
        std::cout << std::format(kFormat, key, value);

    std::cout << "  Weights:\n";
    for (const auto& [name, list, weight] : weight_ranges) {
        std::cout << "    " << name << ":\n";
        if (weight != kOther) {
            for (auto iter : list) {
            std::cout << std::format(kFormat, iter, this->weight_table.at(iter));
            }
        } else {
            for (auto iter : list) {
                auto view = iter.substr(0, iter.find('='));
                std::cout << std::format(kFormat, view, this->weight_table.at(view));
            }
        }
    }

    std::cout << std::format("{:=^80}\n", "");
}

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
    if (config.option_table["detail"]) config.print_in_detail();

    return config;
}


} // namespace dark
