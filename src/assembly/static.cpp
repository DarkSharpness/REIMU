#include <assembly/parser.h>
#include <assembly/storage.h>
#include <assembly/exception.h>
#include <utility.h>

namespace dark {

Alignment::Alignment(std::size_t alignment) : alignment(alignment) {
    throw_if(!std::has_single_bit(alignment), "Invalid alignment: \"{}\"", alignment);
}

IntegerData::IntegerData(std::string_view data, Type type)
    : data(), type(type) {
    runtime_assert(Type::BYTE <= type && type <= Type::LONG);
    auto [value] = Parser {data} .match <Immediate> ();
    this->data = std::move(value);
}

ZeroBytes::ZeroBytes(std::size_t count) : count(count) {}

ASCIZ::ASCIZ(std::string str) : data(std::move(str)) {}

auto sv_to_reg(std::string_view view) -> Register {
    auto reg = sv_to_reg_nothrow(view);
    if (reg.has_value()) return *reg;
    throw FailToParse { std::format("Invalid register: \"{}\"", view) };
}

} // namespace dark
