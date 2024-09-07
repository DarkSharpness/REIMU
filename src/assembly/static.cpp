#include "assembly/storage/static.h"
#include "assembly/exception.h"

namespace dark {

Alignment::Alignment(std::size_t alignment) : alignment(alignment) {
    throw_if(!std::has_single_bit(alignment), "Invalid alignment: \"{}\"", alignment);
}

IntegerData::IntegerData(Immediate data, Type type) : data(std::move(data)), type(type) {}

ZeroBytes::ZeroBytes(std::size_t count) : count(count) {}

ASCIZ::ASCIZ(std::string str) : data(std::move(str)) {}

auto sv_to_reg(std::string_view view) -> Register {
    auto reg = sv_to_reg_nothrow(view);
    if (reg.has_value())
        return *reg;
    throw FailToParse{fmt::format("Invalid register: \"{}\"", view)};
}

} // namespace dark
