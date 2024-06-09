#include <storage.h>
#include <utility.h>
#include <assembly/exception.h>

namespace dark {

/**
 * @brief Just throw FailToParse exception.
*/
[[noreturn]] 
static void throw_invalid(std::string_view msg = "Invalid immediate format") {
    throw FailToParse { std::string(msg) };
}

static bool is_split_char(char c) {
    return std::isspace(c) || c == '+' || c == '-' || c == '(' || c == ')';
}

static bool is_label_char(char c) {
    return std::isalnum(c) || c == '_' || c == '.' || c == '@';
}

static auto parse_relocation(std::string_view view) -> std::unique_ptr <ImmediateBase> {
    using enum RelImmediate::Operand;
    if (view.starts_with("hi(")) {
        return std::make_unique <RelImmediate> (HI, view.substr(2));
    } else if (view.starts_with("lo(")) {
        return std::make_unique <RelImmediate> (LO, view.substr(2));
    } else if (view.starts_with("pcrel_hi(")) {
        return std::make_unique <RelImmediate> (PCREL_HI, view.substr(7));
    } else if (view.starts_with("pcrel_lo(")) {
        return std::make_unique <RelImmediate> (PCREL_LO, view.substr(7));
    } else {
        throw_invalid("Invalid relocation format");
    }
}

static auto parse_integer(std::string_view view) -> std::unique_ptr <ImmediateBase> {
    if (view.starts_with('0')) {
        std::size_t base = [&view]() {
            view.remove_prefix(1);
            if (view.starts_with('x'))
                return view.remove_prefix(1), 16;
            if (view.starts_with('b'))
                return view.remove_prefix(1), 2;
            /* No choice */ return 8;
        }();

        auto number = sv_to_integer <target_size_t> (view, base);
        if (!number.has_value()) throw_invalid("Invalid integer format");

        return std::make_unique <IntImmediate> (*number);
    } else {
        // Utilizing the fact that int64 is enough to cover.
        static_assert(sizeof(target_size_t) == 4);

        // The default value_or is invalid.
        auto number = sv_to_integer <std::int64_t> (view, 10)
            .value_or(std::numeric_limits <std::int64_t>::max());

        // Allow to be negative, but the value should be in range.
        if (number > std::numeric_limits <target_size_t>::max()
         || number < std::numeric_limits <target_ssize_t>::min())
            throw_invalid("Immediate value out of range");

        return std::make_unique <IntImmediate> (static_cast <target_size_t> (number));
    }
}

static auto parse_single(std::string_view view) -> std::unique_ptr <ImmediateBase> {
    if (view.empty()) throw_invalid();
    if (view.front() == '-' || std::isdigit(view.front())) {
        return parse_integer(view);
    } else { // Label case.
        return std::make_unique <StrImmediate> (view);
    }
}

/** May include add/sub expression. */
static auto parse_expression(std::string_view view) -> std::unique_ptr <ImmediateBase> {
    auto pos = std::ranges::find_if(view, is_split_char);
    // Integer or label.
    if (pos == view.end()) return parse_single(view);
    // Negative integer.
    if (pos == view.begin() && *pos == '-') return parse_integer(view);

    throw_invalid("Invalid expression format");
}

static auto parse_immediate(std::string_view view) -> std::unique_ptr <ImmediateBase> {
    while (!view.empty() && std::isspace(view.front()))
        view.remove_prefix(1);
    while (!view.empty() && std::isspace(view.back()))
        view.remove_suffix(1);

    if (view.empty()) throw_invalid();
    if (view.starts_with('%')) {
        return parse_relocation(view.substr(1));
    } else {
        if (view.starts_with('(')) {
            if (!view.ends_with(')')) throw_invalid();
            return parse_immediate(view.substr(1, view.size() - 2));
        }
        return parse_expression(view);
    }
}

Immediate::Immediate(std::string_view view) : data(parse_immediate(view)) {}

std::string Immediate::to_string() const {
    throw_invalid();
}


} // namespace dark
