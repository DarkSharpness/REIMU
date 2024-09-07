#pragma once
#include <memory>
#include <span>
#include <string_view>

namespace dark {

struct unique_string {
private:
    std::unique_ptr<char[]> _M_data; // data of the string
    std::size_t _M_size;             // size of the string
public:
    unique_string() : _M_data(), _M_size() {}
    unique_string(std::string_view str) :
        _M_data(std::make_unique<char[]>(str.size())), _M_size(str.size()) {
        std::copy(str.begin(), str.end(), _M_data.get());
    }

    std::string_view to_sv() const { return {_M_data.get(), _M_size}; }
    std::span<char> to_span() { return {_M_data.get(), _M_size}; }

    unique_string(unique_string &&)            = default;
    unique_string &operator=(unique_string &&) = default;

    char operator[](std::size_t i) const { return _M_data[i]; }
    char &operator[](std::size_t i) { return _M_data[i]; }
    size_t length() const { return _M_size; }
};

} // namespace dark
