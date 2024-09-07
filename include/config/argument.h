#pragma once
#include "fmtlib.h"
#include <functional>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace dark {

struct ArgumentParser {
public:
    explicit ArgumentParser(int argc, char **argv);

    enum class Rule {
        KeyOnly,  // Key + no value
        KeyValue, // Key + value
    };

    template <Rule rule, typename... _Args>
    [[nodiscard]]
    auto match(std::initializer_list<std::string_view> list, _Args &&...args) {
        auto result = this->match_one(list);
        if constexpr (rule == Rule::KeyOnly) {
            return this->match_k(result, std::forward<_Args>(args)...);
        } else if constexpr (rule == Rule::KeyValue) {
            return this->match_kv(result, std::forward<_Args>(args)...);
        } else {
            static_assert(sizeof...(_Args) + 1 == 0, "Invalid rule");
        }
    }

    const auto &get_map() const { return this->kv_map; }

private:
    std::unordered_map<std::string_view, std::string_view> kv_map;
    using _Match_Result_t = std::optional<typename decltype(kv_map)::iterator>;

    [[noreturn]]
    static void handle(std::string_view msg);

    template <typename _Tp, typename... _Args>
    static void check(_Tp &&cond, fmt::format_string<_Args...> fmt = "", _Args &&...args) {
        if (!cond) {
            handle(fmt::format(fmt, std::forward<_Args>(args)...));
        }
    }

    auto match_one(std::initializer_list<std::string_view> list) -> _Match_Result_t {
        _Match_Result_t result;
        for (auto key : list) {
            if (auto it = kv_map.find(key); it != kv_map.end()) {
                if (result) {
                    auto old_key = result.value()->first;
                    this->check(false, "Duplicate option: {} and {}", old_key, key);
                }
                result = it;
            }
        }
        return result;
    }

    template <typename _Fn>
    auto match_k(_Match_Result_t result, _Fn &&fn) -> void {
        if (result.has_value()) {
            auto iter         = result.value();
            auto [key, value] = *iter;
            this->check(value.empty(), "Unexpected value for option {}: {}", key, value);
            this->kv_map.erase(iter);
            std::invoke(std::forward<_Fn>(fn));
        }
    }

    auto match_kv(_Match_Result_t result) -> std::optional<std::string_view> {
        std::optional<std::string_view> retval;
        if (result.has_value()) {
            auto iter         = result.value();
            auto [key, value] = *iter;
            this->kv_map.erase(iter);
            this->check(!value.empty(), "Missing value for option {}", key);
            retval = value;
        }
        return retval;
    }
};

} // namespace dark