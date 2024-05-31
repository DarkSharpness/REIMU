#include "utility.h"
#include <iostream>
#include <unordered_set>

int main(int argc, char** argv) {
    const auto config = dark::Config::parse(argc, argv);
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

    constexpr auto __help = [](std::string_view view) {
        panic_if(view != "-h" && view != "-help", kInvalid, view);
        std::cout << "Usage: simulator [options]\n"
                     "Options:\n"
                     "  -h, --help  Display help information\n"
                     "  -v, --version  Display version information\n";
        std::exit(EXIT_SUCCESS);
    };

    constexpr auto __version = [](std::string_view view) {
        panic_if(view != "-v" && view != "-version", kInvalid, view);
        std::cout << "Version: 0.1.0\n";
        std::exit(EXIT_SUCCESS);
    };

    const auto __set_option = [&](std::string_view view, bool enable) {
        if (view.starts_with("able-")) {
            view = view.substr(5);
        } else if (view.starts_with("-")) {
            view = view.substr(1);
        } else {
            view = {};
        }

        panic_if(view.empty(), kInvalid, view);
        static constexpr const char kDuplicate[] = "Duplicate option: {}";
        auto &table = config.option_table;
        panic_if(table.try_emplace(view, enable).second == false, kDuplicate, view);
    };

    const auto __set_weight = [&](std::string_view view) {
        static constexpr const char kInvalid[]      = "Invalid weight argument: {}";
        static constexpr const char kMissing[]      = "Missing weight: {}";
        static constexpr const char kDuplicate[]    = "Duplicate weight: {}";
        static constexpr const char kNonNegative[]  = "Weight must be non-negative integer: {}";
        static constexpr const char kOverflow[]     = "Weight overflow: {} \n"
                                                      "  Hint: Maximum Weight = {}";
        static constexpr std::size_t kThreshold     = 1919810;

        if (view.starts_with("-w")) {
            view = view.substr(2);
        } else if (view.starts_with("-weight-")) {
            view = view.substr(8);
        } else {
            panic_if(true, kInvalid, view);
        }

        std::size_t next = view.find_first_of('=');
        panic_if(next == std::string_view::npos, kInvalid, view);

        std::string_view name = view.substr(0, next);
        auto weight = __to_size_t(view.substr(next + 1));

        if (std::holds_alternative <CastError> (weight)) {
            switch (std::get <CastError> (weight)) {
                case CastError::Invalid:
                    panic_if(true, kNonNegative, view);
                case CastError::Overflow:
                    panic_if(true, kOverflow, view, kThreshold);
                case CastError::Missing:
                    panic_if(true, kMissing, name);
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
                    panic_if(true, kNonNegative, view);
                case CastError::Overflow:
                    panic_if(true, kOverflow, view, kThreshold);
                case CastError::Missing:
                    panic_if(true, kMissing, view);
                default:
                    runtime_assert(false);
            }
        } else {
            panic_if(config.maximum_time != config.uninitialized, kDuplicate, view);
            auto value = std::get <std::size_t> (timeout);
            config.maximum_time = value;
        }
    };

    if (argc == 1) __help("-h");

    for (int i = 1 ; i < argc ; ++i) {
        const std::string_view view = argv[i];
        if (view.empty() || view.front() != '-') {
            panic_if(true, kInvalid, view);
        } else if (view.starts_with("-h")) {
            __help(view);
        } else if (view.starts_with("-v")) {
            __version(view);
        } else if (view.starts_with("-en")) {
            __set_option(view.substr(3), true);
        } else if (view.starts_with("-dis")) {
            __set_option(view.substr(4), false);
        } else if (view.starts_with("-w")) {
            __set_weight(view);
        } else if (view.starts_with("-time=")) {
            __set_timeout(view.substr(6));
        } else {
            panic_if(true, kInvalid, view);
        }
    }

    config.initialize_with_check();
    if (config.option_table["detail"])
        config.print_in_detail();

    return config;
}

} // namespace dark
