#include "utility.h"
#include "config.h"

namespace dark {

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

} // namespace dark
