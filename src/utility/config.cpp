#include "utility/error.h"
#include <string>
#include <string_view>
#include <utility.h>
#include <utility/cast.h>
#include <config/config.h>
#include <config/default.h>
#include <config/weight.h>
#include <config/argument.h>
#include <unordered_set>
#include <unordered_map>
#include <fstream>
#include <memory>
#include <vector>
#include <iostream>
#include <sstream>
#include <filesystem>

namespace dark {

template <typename... _Args>
[[noreturn]]
static void handle_error(std::format_string <_Args...> fmt, _Args &&...args) {
    panic("Fail to parse command line argument.\n  {}",
        std::format(fmt, std::forward <_Args> (args)...));
}

void ArgumentParser::handle(std::string_view str) {
    panic("Fail to parse command line argument.\n  {}", str);
}

using _Option_Set_t = std::unordered_set <std::string_view>;
using _Weight_Map_t = std::unordered_map <std::string_view, std::size_t>;

struct InputFile {
    mutable std::istream *stream;
    std::unique_ptr <std::ifstream> owning;
    std::string_view name;

    explicit InputFile(std::string_view name) : name(name) {
        if (this->name == config::kStdin) {
            this->stream = &std::cin;
        } else {
            this->owning = std::make_unique <std::ifstream> (std::string(this->name));
            this->stream = this->owning.get();
        }
    }
};

struct OutputFile {
    mutable std::ostream *stream;
    std::unique_ptr <std::ofstream> owning;
    std::string_view name;

    explicit OutputFile(std::string_view name) : name(name) {
        if (this->name == config::kStdout) {
            this->stream = &std::cout;
        } else if (this->name == config::kStderr) {
            this->stream = &std::cerr;
        } else {
            this->owning = std::make_unique <std::ofstream> (std::string(this->name));
            this->stream = this->owning.get();
        }
    }
};

struct OJInfo {
    std::unique_ptr <std::ostringstream> error;
    std::unique_ptr <std::ostringstream> output;
    std::unique_ptr <std::ostringstream> profile;
};

struct Config_Impl {
    const InputFile  input;      // Program input
    const OutputFile output;     // Program output
    const OutputFile profile;    // Profile output
    const std::string_view answer;  // Answer file

    const std::size_t max_timeout = {};       // Maximum time
    const std::size_t memory_size = {};       // Memory storage 
    const std::size_t stack_size  = {};       // Maximum stack

    const std::vector <std::string_view> assembly_files;  // Assembly files

    // The additional configuration table provided by the user.
    _Option_Set_t option_table;
    // The additional weight table provided by the user.
    _Weight_Map_t weight_table;

    OJInfo oj_data;

    explicit Config_Impl(ArgumentParser &parser);

    bool has_option(std::string_view) const;
    void add_option(std::string_view);
    void print_in_detail() const;
    void initialize_with_check();
    void initialize_configuration();
    void oj_handle();
};

struct Config::Impl : Config, Config_Impl {
    explicit Impl(ArgumentParser &parser) : Config_Impl(parser) {}
    using Config_Impl::has_option;
};

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

static void check_invalid_weight(_Weight_Map_t &table) {
    _Weight_Map_t default_weights {
        std::begin(kWeightList), std::end(kWeightList)
    };

    for (const auto &[key, value] : table) {
        // Name of a specific weight, ok.
        if (default_weights.count(key) != 0) continue; 
        // Name of a range weight, and set all the weights in the range.
        if (auto list = find_weight_range(key)) {
            for (const auto &name : *list) default_weights[name] = value;
        } else {
            handle_error("Unknown weight: {}", key);
        }
    }

    for (const auto &pair : default_weights) table.insert(pair);
}

static void check_duplicate_files(std::span <const std::string_view> files) {
    std::unordered_set <std::string_view> existing_files;
    if (files.empty()) {
        handle_error("No assembly file is provided.");
    }

    for (const auto &name : files) {
        if (!existing_files.insert(name).second) {
            handle_error("Duplicate assembly file: {}", name);
        }
    }
}

static auto get_integer(const std::string_view str, std::string_view what) -> std::size_t {
    int base = 10;
    std::string_view view = str;
    if (str.starts_with("0x")) {
        base = 16;
        view = str.substr(2);
    }
    if (auto val = sv_to_integer <std::size_t> (view, base)) {
        return *val;
    } else {
        handle_error("{} must be a non-negative integer: {}", what, str);
    }
}

[[maybe_unused]]
static auto get_memory(std::string_view str, std::string_view what) -> std::size_t {
    std::size_t factor = 1;
    if (str.ends_with('K') || str.ends_with('k')) {
        factor = std::size_t(1) << 10;
        str.remove_suffix(1);
    } else if (str.ends_with('M') || str.ends_with('m')) {
        factor = std::size_t(1) << 20;
        str.remove_suffix(1);
    }
    return get_integer(str, what) * factor;
}

[[maybe_unused]]
static auto get_files(std::string_view str) -> std::vector <std::string_view> {
    std::vector <std::string_view> files;
    std::size_t next = str.find_first_of(',');
    while (next != std::string_view::npos) {
        files.emplace_back(str.substr(0, next));
        str.remove_prefix(next + 1);
        next = str.find_first_of(',');
    }
    files.emplace_back(str);
    return files;
}

static auto make_memory_string(std::size_t size) -> std::string {
    constexpr std::size_t kMin = (std::size_t(1) << 20) / 10;
    if (size < kMin) {
        return std::format("({:.2f} KB)", double(size) / 1024);
    } else {
        return std::format("({:.2f} MB)", double(size) / (1024 * 1024));
    }
}

using enum ArgumentParser::Rule;
/**
 * Core implementation of the configuration parser.
 */
Config_Impl::Config_Impl(ArgumentParser &parser) :
    input  (parser.match<KeyValue>({"-i", "--input"})  .value_or(config::kInitStdin)),
    output (parser.match<KeyValue>({"-o", "--output"}) .value_or(config::kInitStdout)),
    profile(parser.match<KeyValue>({"-p", "--profile"}).value_or(config::kInitProfile)),
    answer (parser.match<KeyValue>({"-a", "--answer"}) .value_or(config::kInitAnswer)),
    max_timeout(parser.match<KeyValue>({"-t", "--time"})
        .transform([](std::string_view str) { return get_integer(str, "--time"); })
        .value_or(config::kInitTimeOut)),
    memory_size(parser.match<KeyValue>({"-m", "--memory"})
        .transform([](std::string_view str) { return get_memory(str, "--memory"); })
        .value_or(config::kInitMemorySize)),
    stack_size(parser.match<KeyValue>({"-s", "--stack"})
        .transform([](std::string_view str) { return get_memory(str, "--stack"); })
        .value_or(config::kInitStackSize)),
    assembly_files(parser.match<KeyValue>({"-f", "--file"})
        .transform(get_files)
        .value_or(config::kInitAssemblyFiles)),
    option_table(),
    weight_table()
{
    for (auto option : config::kSupportedOptions)
        parser.match<KeyOnly>({option}, [this, option]() {
            // substr(2) to remove "--" prefix
            this->add_option(option.substr(2));
        });

    for (auto [name, weight] : parser.get_map()) {
        std::string_view what;
        if (name.starts_with("--weight-")) {
            what = name.substr(9);
        } else if (name.starts_with("-w")) {
            what = name.substr(2);
        } else {
            handle_error("Unknown command line argument: {}", name);
        }
        auto [_, sucess] = this->weight_table.try_emplace(what, get_integer(weight, "weight"));
        if (!sucess) handle_error("Duplicate weight: {}", what);
    }

    // Binding io, and check the configuration.
    this->initialize_with_check();
}

bool Config_Impl::has_option(std::string_view name) const {
    return this->option_table.contains(name);
}

void Config_Impl::add_option(std::string_view name) {
    this->option_table.insert(name);
}

void Config_Impl::initialize_with_check() {
    if (this->stack_size > this->memory_size)
        handle_error("Stack size exceeds memory size: "
            "0x{:x} > 0x{:x}", this->stack_size, this->memory_size);

    check_duplicate_files(this->assembly_files);
    check_invalid_weight(this->weight_table);

    console::profile.rdbuf(this->profile.stream->rdbuf());

    this->initialize_configuration();
}

void Config_Impl::initialize_configuration() {
    const auto __silent = [&] {
        console::warning.rdbuf(nullptr);
        console::message.rdbuf(nullptr);
        console::profile.rdbuf(nullptr);
    };

    const auto __all = [&] {
        this->add_option("cache");
        this->add_option("predictor");
    };

    const auto __oj_mode = [&] {
        __all();

        this->oj_data.error = std::make_unique <std::ostringstream> ();
        this->oj_data.output = std::make_unique <std::ostringstream> ();
        this->oj_data.profile = std::make_unique <std::ostringstream> ();

        console::warning.rdbuf(nullptr);
        console::message.rdbuf(nullptr);
        console::error.rdbuf(this->oj_data.error->rdbuf());
        console::profile.rdbuf(this->oj_data.profile->rdbuf());

        this->output.stream = this->oj_data.output.get();
    };

    const auto __detail = [&] {
        if (this->has_option("silent"))
            handle_error("Cannot use --detail with --silent.");
        this->print_in_detail();
    };

    // OJ-mode overrides all other options.
    if (this->has_option("oj-mode")) return __oj_mode();
    if (this->has_option("silent"))  __silent();
    if (this->has_option("detail"))  __detail();
    if (this->has_option("all"))     __all();
}

void Config_Impl::print_in_detail() const {
    using console::message;

    message << std::format("\n{:=^80}\n\n", " Configuration details ");
    message << std::format("  Input file: {}\n", this->input.name);
    message << std::format("  Output file: {}\n", this->output.name);
    message << std::format("  Assembly files: ");

    for (const auto& file : this->assembly_files)
        message << file << ' ';

    message << std::format("\n"
        "  Memory size: {} bytes {}\n"
        "  Stack  size: {} bytes {}\n",
        this->memory_size, make_memory_string(this->memory_size),
        this->stack_size, make_memory_string(this->stack_size));

    if (this->max_timeout == config::kInitTimeOut) {
        message << "  Maximum time: no limit\n";
    } else {
        message << std::format("  Maximum time: {} cycles\n", this->max_timeout);
    }

    // Format string for printing options and weights
    static constexpr char kFormat[] = "    - {:<8} = {}\n";

    message << "  Options:\n";
    for (const auto &key : config::kSupportedOptions)
        // substr(2) to remove "--" prefix
        message << std::format(kFormat, key.substr(2), this->has_option(key));

    message << "  Weights:\n";
    for (const auto &[name, list, weight] : kWeightRanges) {
        message << "    " << name << ":\n";
        if (weight != kManual) {
            for (auto iter : list) {
                message << std::format(kFormat, iter, this->weight_table.at(iter));
            }
        } else {
            for (auto iter : list) {
                auto name = parse_manual(iter).first;
                message << std::format(kFormat, name, this->weight_table.at(name));
            }
        }
    }

    message << std::format("\n{:=^80}\n\n", "");
}

static auto read_answer(std::string_view name) -> std::string {
    std::string path { name };
    const auto file_size = std::filesystem::file_size(path);
    std::string retval;
    retval.resize(file_size);
    std::ifstream file { path, std::ios::binary };
    file.read(retval.data(), file_size);
    return retval;
}

void Config_Impl::oj_handle() {
    // using console::message;
    auto error_str = std::move(*this->oj_data.error).str();
    if (!error_str.empty()) {
        std::cerr << "Error: " << error_str;
        return;
    }

    auto output_str = std::move(*this->oj_data.output).str();
    auto answer_str = read_answer(this->answer);

    if (output_str != answer_str) {
        std::cerr << "Wrong answer.\n";
        return;
    }

    // auto profile_str = std::move(*this->oj_data.profile).str();
}

/* The commands below are just forwarded to impl.  */

auto Config::parse(int argc, char** argv) -> std::unique_ptr <Config> {
    using console::message;
    ArgumentParser parser { argc, argv };

    parser.match<KeyOnly>({"-h", "--help"}, []() { 
        message << config::kHelpMessage;
        std::exit(EXIT_SUCCESS);
    });

    parser.match<KeyOnly>({"-v", "--version"}, []() {
        message << config::kVersionMessage;
        std::exit(EXIT_SUCCESS);
    });

    return std::unique_ptr <Config> (new Impl { parser });
}

auto Config::has_option(std::string_view name) const -> bool {
    return this->get_impl().has_option(name);
}

auto Config::get_impl() const -> const Impl & {
    return *static_cast <const Impl*> (this);
}

auto Config::get_input_stream() const -> std::istream& {
    return *this->get_impl().input.stream;
}

auto Config::get_output_stream() const -> std::ostream& {
    return *this->get_impl().output.stream;
}

auto Config::get_stack_top() const -> target_size_t {
    return this->get_impl().memory_size;
}

auto Config::get_stack_low() const -> target_size_t {
    return this->get_impl().memory_size - this->get_impl().stack_size;
}

auto Config::get_timeout() const -> std::size_t {
    return this->get_impl().max_timeout;
}

auto Config::get_assembly_names() const -> std::span <const std::string_view> {
    return this->get_impl().assembly_files;
}

auto Config::get_weight(std::string_view name) const -> std::size_t {
    return this->get_impl().weight_table.at(name);
}

Config::~Config() {
    auto *impl_ptr = static_cast <Impl*> (this);
    if (this->has_option("oj-mode"))
        impl_ptr->oj_handle();

    std::destroy_at <Config_Impl> (impl_ptr);
}

} // namespace dark
