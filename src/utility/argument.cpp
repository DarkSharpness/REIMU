#include "config/argument.h"

namespace dark {

/* Just get exactly one argument  */
static auto get_args(std::string_view arg) -> std::pair<std::string_view, std::string_view> {
    auto pos = arg.find("=");
    if (pos != arg.npos) {
        auto option = arg.substr(0, pos);
        auto value  = arg.substr(pos + 1);
        return {option, value};
    } else {
        return {arg, ""};
    }
}

/**
 * @brief Construct a new Argument Parser:: Argument Parser object
 * @param argc argument count from main
 * @param argv argument vector from main
 */
ArgumentParser::ArgumentParser(int argc, char **argv) {
    using _Element = decltype(this->kv_map)::value_type;
    _Element *last = nullptr;
    for (int i = 1; i < argc; i++) {
        std::string_view arg = argv[i];
        if (arg.starts_with('-')) {
            auto [key, value]    = get_args(arg);
            auto [iter, success] = this->kv_map.try_emplace(key, value);
            this->check(success, "Duplicate option: {}", key);
            last = &(*iter);
        } else {
            this->check(last != nullptr, "First argument must be an option: {}", arg);
            auto &[key, value] = *last;
            this->check(
                value.empty(), "Value for option \"{}\" is already set as \"{}\"", key, value
            );
            value = arg;
        }
    }
}


} // namespace dark
