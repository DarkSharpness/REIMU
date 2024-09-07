#include "config/config.h"
#include "config/argument.h"
#include "config/counter.h"
#include "config/default.h"
#include "utility/cast.h"
#include "utility/error.h"
#include "utility/tagged.h"
#include <cctype>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace dark {

template <typename... _Args>
[[noreturn]]
static void handle_error(fmt::format_string<_Args...> fmt, _Args &&...args) {
    try {
        panic(
            "Fail to parse command line argument.\n  {}",
            fmt::format(fmt, std::forward<_Args>(args)...)
        );
    } catch (dark::PanicError) { std::exit(EXIT_FAILURE); }
}

void ArgumentParser::handle(std::string_view str) {
    try {
        panic("Fail to parse command line argument.\n  {}", str);
    } catch (dark::PanicError) { std::exit(EXIT_FAILURE); }
}

using _Option_Set_t = std::unordered_set<std::string_view>;
using _Weight_Map_t = std::unordered_map<std::string_view, std::size_t>;
using weight::Counter;

struct InputFile {
    explicit InputFile(std::string_view name) : name(name) {}

    auto get_name() const -> std::string_view { return this->name; }

    auto get_file_name() const -> std::optional<std::string_view> {
        using _Option_t = std::optional<std::string_view>;
        return this->name == config::kStdin ? _Option_t{} : this->name;
    }

    auto get_stream() const -> std::istream & {
        runtime_assert(this->stream != nullptr);
        return *this->stream;
    }

    auto init() -> void {
        runtime_assert(this->stream == nullptr);
        if (this->name == config::kStdin) {
            this->stream = &std::cin;
        } else {
            this->owning = std::make_unique<std::ifstream>(std::string(this->name));
            if (!this->owning->good()) {
                handle_error("Fail to open input file: {}", this->name);
            }
            this->stream = this->owning.get();
        }
    }

    auto try_init() -> bool {
        if (this->stream == nullptr) {
            this->init();
            return true;
        }
        return false;
    }

private:
    std::istream *stream{};
    std::unique_ptr<std::ifstream> owning;
    std::string_view name;
};

struct OutputFile {
    explicit OutputFile(std::string_view name) : name(name) {}

    auto get_name() const -> std::string_view { return this->name; }

    auto get_file_name() const -> std::optional<std::string_view> {
        using _Option_t = std::optional<std::string_view>;
        return this->name == config::kStdout || this->name == config::kStderr ? _Option_t{}
                                                                              : this->name;
    }

    auto get_stream() const -> std::ostream & {
        runtime_assert(this->stream != nullptr);
        return *this->stream;
    }

    auto init_stream() -> void {
        runtime_assert(this->stream == nullptr);
        if (this->name == config::kStdout) {
            this->stream = &std::cout;
        } else if (this->name == config::kStderr) {
            this->stream = &std::cerr;
        } else {
            this->owning = std::make_unique<std::ofstream>(std::string(this->name));
            if (!this->owning->good()) {
                handle_error("Fail to open output file: {}", this->name);
            }
            this->stream = this->owning.get();
        }
    }

    auto init_stream(std::ostream *stream) -> void {
        runtime_assert(this->stream == nullptr);
        this->stream = stream;
    }

    auto try_init() -> bool {
        if (this->stream == nullptr) {
            this->init_stream();
            return true;
        }
        return false;
    }

private:
    std::ostream *stream{};
    std::unique_ptr<std::ofstream> owning;
    std::string_view name;
};

struct OJInfo {
    std::unique_ptr<std::ostringstream> error;
    std::unique_ptr<std::ostringstream> output;
    std::unique_ptr<std::ostringstream> profile;
};

struct Config_Impl {
    InputFile input;               // Program input
    OutputFile output;             // Program output
    OutputFile profile;            // Profile output
    const std::string_view answer; // Answer file

    const std::size_t max_timeout = {}; // Maximum time
    const std::size_t memory_size = {}; // Memory storage
    const std::size_t stack_size  = {}; // Maximum stack

    const std::vector<std::string_view> assembly_files; // Assembly files

    // The additional configuration table provided by the user.
    _Option_Set_t option_table;
    // The counter for the weight.
    Counter counter;

    OJInfo oj_data;

    _Weight_Map_t weight_table;

    explicit Config_Impl(ArgumentParser &parser);

    bool has_option(std::string_view) const;
    void add_option(std::string_view);
    void print_in_detail() const;
    void initialize();
    void initialize_with_check();
    void initialize_configuration();
    void initialize_iostream();
    void oj_handle();
};

struct Config::Impl : Config, Config_Impl {
    explicit Impl(ArgumentParser &parser) : Config_Impl(parser) {}
    using Config_Impl::has_option;
    ~Impl() {
        if (this->has_option("oj-mode"))
            this->oj_handle();
    }
};

template <typename _Tp>
concept CounterType = requires(_Tp &a, std::size_t s) {
    { a.kDefaultWeight } -> std::same_as<const std::size_t &>;
    { a.kName } -> std::same_as<const std::string_view &>;
    { a.kMembers[0] } -> std::same_as<const std::string_view &>;
    { a.get_weight() } -> std::same_as<std::size_t>;
    a.set_weight(s);
} && sizeof(_Tp) == sizeof(std::size_t);

template <CounterType _Tp>
static void insert_weight(_Tp &counter, _Weight_Map_t &table) {
    static constexpr auto array = []() {
        std::array<char, _Tp::kName.size() + 1> buffer{};
        for (std::size_t i = 0; i < _Tp::kName.size(); ++i) {
            char c = _Tp::kName[i];
            if (c >= 'A' && c <= 'Z')
                c = c + ('a' - 'A');
            buffer[i] = c;
        }
        return buffer;
    }();
    constexpr auto kName = std::string_view{array.data(), _Tp::kName.size()};
    if (auto iter = table.find(kName); iter == table.end()) {
        counter.set_weight(_Tp::kDefaultWeight);
    } else {
        counter.set_weight(iter->second);
        table.erase(iter);
    }
}

static void check_invalid_weight(Counter &counter, _Weight_Map_t &table) {
    visit([&](auto &counter) { insert_weight(counter, table); }, counter);
    if (!table.empty()) {
        handle_error("Unknown weight: {}", table.begin()->first);
    }
}

static void check_duplicate_files(
    std::span<const std::string_view> files, std::optional<std::string_view> input_file,
    std::optional<std::string_view> answer_file, std::optional<std::string_view> output_file,
    std::optional<std::string_view> profile_file
) {
    std::unordered_set<std::string_view> existing_files;
    if (files.empty()) {
        handle_error("No assembly file is provided.");
    }

    for (const auto &name : files) {
        if (!existing_files.insert(name).second) {
            handle_error("Duplicate assembly file: {}", name);
        }
    }

    auto input_files = std::move(existing_files);

    if (input_file)
        input_files.insert(*input_file);
    if (answer_file)
        input_files.insert(*answer_file);

    constexpr auto kOutputOverlapInput =
        "File {} is both used as program input and program output. Potential overwrite!";
    constexpr auto kProfileOverlapInput =
        "File {} is both used as program input and profile output. Potential overwrite!";
    constexpr auto kOutputOverlapProfile =
        "File {} is both used as program output and profile output. Potential overwrite!";

    if (output_file) {
        if (input_files.contains(*output_file)) {
            handle_error(kOutputOverlapInput, *output_file);
        }
    }

    if (profile_file) {
        if (input_files.contains(*profile_file)) {
            handle_error(kProfileOverlapInput, *profile_file);
        }
    }

    if (output_file && profile_file && *output_file == *profile_file) {
        handle_error(kOutputOverlapProfile, *output_file);
    }
}

static auto get_integer(const std::string_view str, std::string_view what) -> std::size_t {
    int base              = 10;
    std::string_view view = str;
    if (str.starts_with("0x")) {
        base = 16;
        view = str.substr(2);
    }
    if (auto val = sv_to_integer<std::size_t>(view, base)) {
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
static auto get_files(std::string_view str) -> std::vector<std::string_view> {
    std::vector<std::string_view> files;
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
        return fmt::format("({:.2f} KB)", double(size) / 1024);
    } else {
        return fmt::format("({:.2f} MB)", double(size) / (1024 * 1024));
    }
}

using enum ArgumentParser::Rule;
/**
 * Core implementation of the configuration parser.
 */
Config_Impl::Config_Impl(ArgumentParser &parser) :
    input(parser.match<KeyValue>({"-i", "--input"}).value_or(config::kInitStdin)),
    output(parser.match<KeyValue>({"-o", "--output"}).value_or(config::kInitStdout)),
    profile(parser.match<KeyValue>({"-p", "--profile"}).value_or(config::kInitProfile)),
    answer(parser.match<KeyValue>({"-a", "--answer"}).value_or(config::kInitAnswer)),
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
    option_table() {
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
        auto [_, success] = this->weight_table.try_emplace(what, get_integer(weight, "weight"));
        if (!success)
            handle_error("Duplicate weight: {}", what);
    }
}

bool Config_Impl::has_option(std::string_view name) const {
    return this->option_table.contains(name);
}

void Config_Impl::add_option(std::string_view name) {
    this->option_table.insert(name);
}

void Config_Impl::initialize() {
    // Check input arguments.
    this->initialize_with_check();
    // Initialize configuration.
    this->initialize_configuration();
    // Initialize input and output stream.
    this->initialize_iostream();
}

void Config_Impl::initialize_with_check() {
    if (this->stack_size > this->memory_size)
        handle_error(
            "Stack size exceeds memory size: "
            "0x{:x} > 0x{:x}",
            this->stack_size, this->memory_size
        );

    check_invalid_weight(this->counter, this->weight_table);
    check_duplicate_files(
        this->assembly_files, this->input.get_file_name(),
        // Remark: answer file is only useful in OJ mode for now.
        this->has_option("oj-mode") ? this->answer : std::optional<std::string_view>{},
        this->output.get_file_name(), this->profile.get_file_name()
    );
}

void Config_Impl::initialize_iostream() {
    this->input.try_init();
    this->output.try_init();
    if (this->profile.try_init())
        console::profile.rdbuf(this->profile.get_stream().rdbuf());
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
        constexpr auto kOutputOverlapAnswer =
            "File {} is both used as program output and answer file. Potential overwrite!";
        constexpr auto kProfileOverlapAnswer =
            "File {} is both used as profile output and answer file. Potential overwrite!";

        if (auto file = this->output.get_file_name()) {
            if (*file == this->answer) {
                handle_error(kOutputOverlapAnswer, *file);
            }
        }
        if (auto file = this->profile.get_file_name()) {
            if (*file == this->answer) {
                handle_error(kProfileOverlapAnswer, *file);
            }
        }

        __all();
        __silent();

        this->oj_data.error   = std::make_unique<std::ostringstream>();
        this->oj_data.output  = std::make_unique<std::ostringstream>();
        this->oj_data.profile = std::make_unique<std::ostringstream>();

        console::error.rdbuf(this->oj_data.error->rdbuf());
        console::profile.rdbuf(this->oj_data.profile->rdbuf());

        this->output.init_stream(this->oj_data.output.get());
        this->profile.init_stream(this->oj_data.profile.get());
    };

    const auto __detail = [&] {
        if (this->has_option("silent"))
            handle_error("Cannot use --detail with --silent.");
        this->print_in_detail();
    };

    // OJ-mode overrides all other options.
    if (this->has_option("oj-mode"))
        return __oj_mode();
    if (this->has_option("silent"))
        __silent();
    if (this->has_option("detail"))
        __detail();
    if (this->has_option("all"))
        __all();
}

void Config_Impl::print_in_detail() const {
    warning("Deprecated function: Config_Impl::print_in_detail");
    using console::message;

    message << fmt::format("\n{:=^80}\n\n", " Configuration details ");
    message << fmt::format("  Input file: {}\n", this->input.get_name());
    message << fmt::format("  Output file: {}\n", this->output.get_name());
    message << fmt::format("  Assembly files: ");

    for (const auto &file : this->assembly_files)
        message << file << ' ';

    message << fmt::format(
        "\n"
        "  Memory size: {} bytes {}\n"
        "  Stack  size: {} bytes {}\n",
        this->memory_size, make_memory_string(this->memory_size), this->stack_size,
        make_memory_string(this->stack_size)
    );

    if (this->max_timeout == config::kInitTimeOut) {
        message << "  Maximum time: no limit\n";
    } else {
        message << fmt::format("  Maximum time: {} cycles\n", this->max_timeout);
    }

    // Format string for printing options and weights
    static constexpr char kFormat[] = "    - {:<12} = {}\n";

    message << "  Options:\n";
    for (const auto &key : config::kSupportedOptions) {
        auto option = key.substr(2); // substr(2) to remove "--" prefix
        message << fmt::format(kFormat, option, this->has_option(option));
    }

    message << "  Weights:\n";
    visit(
        [&](auto &counter) {
            message << fmt::format(kFormat, counter.kName, counter.get_weight());
        },
        this->counter
    );

    message << fmt::format("\n{:=^80}\n\n", "");
}

static auto read_answer(std::string_view name) -> std::string {
    std::string path{name};
    const auto file_size = std::filesystem::file_size(path);
    std::string retval;
    retval.resize(file_size);
    std::ifstream file{path, std::ios::binary};
    file.read(retval.data(), file_size);
    return retval;
}

void Config_Impl::oj_handle() {
    OutputFile profile{this->profile.get_name()};
    profile.init_stream();
    auto &os = profile.get_stream();

    auto error_str = std::move(*this->oj_data.error).str();
    if (!error_str.empty()) { // Flush error message and exit.
        os << "Wrong answer. (Program crashed)" << std::endl;
        return;
    }

    auto output_str = std::move(*this->oj_data.output).str();
    auto answer_str = read_answer(this->answer);

    while (output_str.ends_with('\n'))
        output_str.pop_back();
    while (answer_str.ends_with('\n'))
        answer_str.pop_back();

    if (output_str != answer_str) { // Flush output and exit.
        os << "Wrong answer. (Output mismatched)" << std::endl;
        return;
    }

    os << "Accepted.\n";

    auto profile_str = std::move(*this->oj_data.profile).str();
    os << profile_str << std::endl;
}

/* The commands below are just forwarded to impl.  */

auto Config::parse(int argc, char **argv) -> unique_t {
    using console::message;
    ArgumentParser parser{argc, argv};

    parser.match<KeyOnly>({"-h", "--help"}, []() {
        message << config::kHelpMessage;
        std::exit(EXIT_SUCCESS);
    });

    parser.match<KeyOnly>({"-v", "--version"}, []() {
        message << config::kVersionMessage;
        std::exit(EXIT_SUCCESS);
    });

    auto retval = std::make_unique<Impl>(parser);

    retval->initialize();

    return retval;
}

auto Config::has_option(std::string_view name) const -> bool {
    return this->get_impl().has_option(name);
}

auto Config::get_impl() const -> const Impl & {
    return *static_cast<const Impl *>(this);
}

auto Config::get_input_stream() const -> std::istream & {
    return this->get_impl().input.get_stream();
}

auto Config::get_output_stream() const -> std::ostream & {
    return this->get_impl().output.get_stream();
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

auto Config::get_assembly_names() const -> std::span<const std::string_view> {
    return this->get_impl().assembly_files;
}

auto Config::get_weight() const -> const Counter & {
    return this->get_impl().counter;
}

} // namespace dark
