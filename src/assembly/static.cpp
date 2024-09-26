#include "assembly/storage/static.h"
#include "assembly/exception.h"

namespace dark {

Alignment::Alignment(LineInfo li, std::size_t alignment) : StaticData(li), alignment(alignment) {
    throw_if(!std::has_single_bit(alignment), "Invalid alignment: \"{}\"", alignment);
}

IntegerData::IntegerData(LineInfo li, Immediate data, Type type) :
    StaticData(li), data(std::move(data)), type(type) {}

ZeroBytes::ZeroBytes(LineInfo li, std::size_t count) : StaticData(li), count(count) {}

ASCIZ::ASCIZ(LineInfo li, std::string str) : StaticData(li), data(std::move(str)) {}

auto sv_to_reg(std::string_view view) -> Register {
    auto reg = sv_to_reg_nothrow(view);
    if (reg.has_value())
        return *reg;
    throw FailToParse{fmt::format("Invalid register: \"{}\"", view)};
}

} // namespace dark
