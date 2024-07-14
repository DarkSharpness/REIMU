#include <utility.h>
#include <config/config.h>
#include <config/default.h>
#include <config/weight.h>
#include <ranges>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <variant>
#include <fstream>
#include <memory>

namespace dark {

using _Option_Set_t = std::unordered_set <std::string_view>;
using _Weight_Map_t = std::unordered_map <std::string_view, std::size_t>;

struct Config_Impl {
    std::istream *input_stream;
    std::ostream *output_stream;

    static constexpr std::size_t uninitialized = std::size_t();

    std::size_t storage_size = uninitialized;       // Memory storage 
    std::size_t maximum_time = uninitialized;       // Maximum time
    std::size_t stack_size   = uninitialized;       // Maximum stack

    std::vector <std::string_view> assembly_files;  // Assembly files

    // The additional configuration table provided by the user.
    _Option_Set_t option_table;
    // The additional weight table provided by the user.
    _Weight_Map_t weight_table;

    std::string_view input_file;    // Input file
    std::string_view output_file;   // Output file

    explicit Config_Impl(int argc, char** argv);

    ~Config_Impl() {
        if (this->input_stream != &std::cin)    delete this->input_stream;
        if (this->output_stream != &std::cout)  delete this->output_stream;
    }

    void initialize_with_check();
    void parse_options();
    void print_in_detail() const;

    bool has_option(std::string_view) const;
};

struct Config::Impl : Config, Config_Impl {
    explicit Impl(int argc, char** argv) : Config_Impl(argc, argv) {}
    using Config_Impl::has_option;
};

Config::~Config() {
    auto *impl_ptr = static_cast <Impl*> (this);
    std::destroy_at <Config_Impl> (impl_ptr);
}

using weight::kWeightRanges;
using weight::kManual;
using weight::kWeightCount;
using weight::parse_manual;

// The weight list is generated automatically by the weight ranges.
static constexpr auto kWeightList = []() {
    using _Pair_t = std::pair <std::string_view, std::size_t>;
    std::array <_Pair_t, kWeightCount> table {};
    std::size_t which {};
    for (const auto &[name, list, weight] : kWeightRanges) {
        if (weight != kManual) {
            for (auto item : list) table[which++] = { item, weight };
        } else {
            for (auto item : list) table[which++] = parse_manual(item);
        }
    }
    if (which != kWeightCount) throw;
    return table;
} ();

/**
 * @return Whether a give name is a name of a weight range.
 */
static auto find_weight_range(std::string_view need)
-> std::optional <std::span <const std::string_view>> {
    for (const auto &[name, list, _] : kWeightRanges)
        if (name == need) return list;
    return std::nullopt;
}

static void check_weight(_Weight_Map_t &table) {
    _Weight_Map_t default_weights {
        std::begin(kWeightList), std::end(kWeightList)
    };

    for (const auto &[key, value] : table) {
        // Name of a specific weight, ok.
        if (default_weights.count(key) != 0) continue; 
        // Name of a range weight, and set all the weights in the range.
        if (auto list = find_weight_range(key); list.has_value()) {
            for (const auto &name : *list) default_weights[name] = value;
        } else {
            panic("Unknown weight: {}", key);
        }
    }

    for (const auto &pair : default_weights) table.insert(pair);
}

static void check_option(_Option_Set_t &table) {
    const _Option_Set_t default_set = {
        std::begin(config::kSupportedOptions),
        std::end  (config::kSupportedOptions)
    };

    for (const auto &key : table)
        panic_if(default_set.count(key) == 0, "Unknown option: {}", key);
}

static void check_names(std::span <std::string_view> files) {
    std::unordered_set <std::string_view> set;
    for (const auto &name : files)
        panic_if(!set.insert(name).second, "Duplicate assembly file: {}", name);
    panic_if(files.empty(), "Missing input assembly file");
}

void Config_Impl::initialize_with_check() {
    if (this->input_file.empty())
        this->input_file = config::kInitStdin;
    if (this->input_file == "<stdin>")
        this->input_stream = &std::cin;
    else
        this->input_stream = new std::ifstream(std::string(this->input_file));

    if (this->output_file.empty())
        this->output_file = config::kInitStdout;
    if (this->output_file == "<stdout>")
        this->output_stream = &std::cout;
    else
        this->output_stream = new std::ofstream(std::string(this->output_file));

    if (this->assembly_files.empty())
        this->assembly_files.assign(
            std::begin(config::kInitAssemblyFiles),
            std::end  (config::kInitAssemblyFiles)
        );

    if (this->maximum_time ==this->uninitialized)
        this->maximum_time = config::kInitTimeOut;

    if (this->storage_size == this->uninitialized)
        this->storage_size = config::kInitMemorySize;

    if (this->stack_size == this->uninitialized)
        this->stack_size = config::kInitStackSize;

    if (this->stack_size > this->storage_size)
        panic("Stack size exceeds memory size: "
              "0x{:x} > 0x{:x}", this->stack_size, this->storage_size);

    check_names(this->assembly_files);
    check_option(this->option_table);
    check_weight(this->weight_table);
}

void Config_Impl::print_in_detail() const {
    std::cout << std::format("\n{:=^80}\n\n", " Configuration details ");

    std::cout << std::format("  Input file: {}\n", this->input_file);
    std::cout << std::format("  Output file: {}\n", this->output_file);
    std::cout << std::format("  Assembly files: ");

    for (const auto& file : this->assembly_files)
        std::cout << file << ' ';

    std::cout << std::format("\n  Storage size: {} bytes ({:.2f} MB)\n",
        this->storage_size, double(this->storage_size) / (1024 * 1024));

    if (this->maximum_time == config::kInitTimeOut) {
        std::cout << "  Maximum time: no limit\n";
    } else {
        std::cout << std::format("  Maximum time: {} cycles\n", this->maximum_time);
    }

    // Format string for printing options and weights
    static constexpr char kFormat[] = "    - {:<8} = {}\n";

    std::cout << "  Options:\n";
    for (const auto &key : config::kSupportedOptions)
        std::cout << std::format(kFormat, key, this->has_option(key));

    std::cout << "  Weights:\n";
    for (const auto &[name, list, weight] : kWeightRanges) {
        std::cout << "    " << name << ":\n";
        if (weight != kManual) {
            for (auto iter : list) {
                std::cout << std::format(kFormat, iter, this->weight_table.at(iter));
            }
        } else {
            for (auto iter : list) {
                auto name = parse_manual(iter).first;
                std::cout << std::format(kFormat, name, this->weight_table.at(name));
            }
        }
    }

    std::cout << std::format("\n{:=^80}\n\n", "");
}

bool Config_Impl::has_option(std::string_view name) const {
    return this->option_table.count(name) != 0;
}

auto Config::parse(int argc, char** argv) -> std::unique_ptr <Config> {
    return std::unique_ptr <Config> (new Impl {argc, argv});
}

auto Config::has_option(std::string_view name) const -> bool {
    return this->get_impl().has_option(name);
}

auto Config::get_impl() const -> const Impl & {
    return *static_cast <const Impl*> (this);
}

auto Config::get_input_stream() const -> std::istream& {
    return *this->get_impl().input_stream;
}

auto Config::get_output_stream() const -> std::ostream& {
    return *this->get_impl().output_stream;
}

auto Config::get_stack_top() const -> target_size_t {
    return this->get_impl().storage_size;
}

auto Config::get_stack_low() const -> target_size_t {
    return this->get_impl().storage_size - this->get_impl().stack_size;
}

auto Config::get_timeout() const -> std::size_t {
    return this->get_impl().maximum_time;
}

auto Config::get_assembly_names() const -> std::span <const std::string_view> {
    return this->get_impl().assembly_files;
}

/**
 * Core implementation of the configuration parser.
 */
Config_Impl::Config_Impl(int argc, char** argv) {
    static constexpr const char kInvalid[] = "Invalid command line argument: {}";

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

  --detail                                  Print the configuration details.
  --debug                                   Use built-in gdb.
  --cache                                   Enable cache simulation.

  -w<name>=<value>, -weight-<name>=<value>  Set weight (cycles) for a specific assembly command.
                                            The name can be either a opcode name or a group name.
                                            Example: -wadd=1 -wmul=3 -wmemory=100 -wbranch=3

  -t=<time>, -time=<time>                   Set maximum time (cycles) for the simulator, default unlimited.

  -m=<mem>, -mem=<mem>, -memory=<mem>       Set memory size (bytes) for the simulator, default 256MB.
                                            We support K/M suffix for kilobytes/megabytes.
                                            Note that the memory has a hard limit of 1GB.
                                            Example: -m=114K -m=514M -mem=1919 -memory=810
  -s=<mem>, -stack=<mem>                    Set stack size (bytes) for the simulator, default 1MB.
                                            We support K/M suffix for kilobytes/megabytes.
                                            Note that the stack has a hard limit of 1GB.

  -i=<file>, -input=<file>                  Set input file for the simulator.
                                            If <file> is <stdin>, the simulator will read from stdin.
  -o=<file>, -output=<file>                 Set output file for the simulator.
                                            If <file> is <stdout>, the simulator will write to stdout.
  -f=<file>,... -file=<file>,...            Set input files as the assembly code.
)";
        std::exit(EXIT_SUCCESS);
    };

    constexpr auto __version = [](std::string_view view) {
        std::cout << "Version: 0.1.0\n";
        std::exit(EXIT_SUCCESS);
    };

    const auto __set_option = [&](std::string_view view) {
        panic_if(view.empty(), "Invalid option: {}", view);
        auto &table = this->option_table;
        panic_if(table.insert(view).second == false, "Duplicate option: {}", view);
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
                    unreachable();
            }
        } else {
            auto &table = this->weight_table;
            auto  value = std::get <std::size_t> (weight);
            panic_if(table.try_emplace(name, value).second == false, kDuplicate, name);
        }
    };

    const auto __set_timeout = [&](std::string_view view) {
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
                    unreachable();
            }
        } else {
            panic_if(this->maximum_time != this->uninitialized, kDuplicate, view);
            this->maximum_time = std::get <std::size_t> (timeout);
        }
    };

    const auto __set_memory = [&](std::string_view view, bool stack = false) {
        static constexpr const char kMissing[]      = "Missing memory: {}";
        static constexpr const char kDuplicate[]    = "Duplicate memory: {}";
        static constexpr const char kNonNegative[]  = "Memory must be non-negative integer: {}";
        static constexpr const char kOverflow[]     = "Memory overflow: {} \n"
                                                      "  Hint: Maximum Memory = {}";
        static constexpr std::size_t kThreshold     = 1 << 30;  // 1GB

        std::size_t factor = 1;
        if (view.ends_with('K') || view.ends_with('k')) {
            factor = 1 << 10;
            view.remove_suffix(1);
        } else if (view.ends_with('M') || view.ends_with('m')) {
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
                    unreachable();
            }
        } else {
            if (stack) {
                panic_if(this->stack_size != this->uninitialized, kDuplicate, view);
                this->stack_size = std::get <std::size_t> (memory) * factor;
            } else {
                panic_if(this->storage_size != this->uninitialized, kDuplicate, view);
                this->storage_size = std::get <std::size_t> (memory) * factor;
            }
        }
    };

    const auto __set_assembly = [&](std::string_view view) {
        panic_if(view.empty(), "Missing input assembly file");
        auto &files = this->assembly_files;
        panic_if(!files.empty(), "Duplicate input assembly file: {}", view);

        std::size_t next = view.find_first_of(',');
        while (next != std::string_view::npos) {
            files.emplace_back(view.substr(0, next));
            view.remove_prefix(next + 1);
            next = view.find_first_of(',');
        }
        files.emplace_back(view);

        std::unordered_set <std::string_view> set;
        for (const auto &name : files) {
            panic_if(set.insert(name).second == false,
                "Duplicate input assembly file: {}", name);
        }
    };

    for (int i = 1 ; i < argc ; ++i) {
        std::string_view view = argv[i];
        if (view.size() < 2 || view.front() != '-') {
            panic(kInvalid, view);
        } else switch (view[1]) {
            case '-':
                if (view == "--help")       __help(view);
                if (view == "--version")    __version(view);
                __set_option(view.substr(2));
                break;

            // Short form options

            case 'h':   __match_string(view, {"-h"});
                        __help(view);     break;
            case 'v':   __match_string(view, {"-v"});
                        __version(view);  break;
            case 'w':   __match_prefix(view, {"-weight-", "-w"});
                        __set_weight(view); break;
            case 't':   __match_prefix(view, {"-time=", "-t="});
                        __set_timeout(view); break;
            case 'm':   __match_prefix(view, {"-memory=", "-mem=", "-m="});
                        __set_memory(view); break;
            case 's':   __match_prefix(view, {"-stack=", "-s="});
                        __set_memory(view, true); break;
            case 'i':   __match_prefix(view, {"-input=", "-i="});
                        panic_if(!this->input_file.empty(), "Duplicate input file: {}", view);
                        this->input_file = view; break;
            case 'o':   __match_prefix(view, {"-output=", "-o="});
                        panic_if(!this->output_file.empty(), "Duplicate output file: {}", view);
                        this->output_file = view; break;
            case 'f':   __match_prefix(view, {"-file=", "-f="});
                        __set_assembly(view); break;

            default:    panic(kInvalid, view);
        }
    }

    this->initialize_with_check();
    this->parse_options();
}

void Config_Impl::parse_options() {
    // Silent will kill all other options.
    if (this->has_option("silent")) {
        this->option_table = { "silent" };
        ::dark::warning_shutdown = true;
        return;
    }
    if (this->has_option("detail")) this->print_in_detail();
}

} // namespace dark
