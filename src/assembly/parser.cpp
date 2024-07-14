#include <utility.h>
#include <assembly/storage.h>
#include <assembly/exception.h>

namespace dark {

/**
 * @brief Just throw FailToParse exception.
*/
[[noreturn]] 
static void throw_invalid(std::string_view msg = "Invalid immediate format") {
    throw FailToParse { std::string(msg) };
}

static bool is_split_real(char c) {
    return c == '+' || c == '-' || c == '(' || c == ')';
}

static bool is_split_char(char c) {
    return std::isspace(c) || is_split_real(c);
}

static auto remove_front_whitespace(std::string_view str) -> std::string_view {
    while (!str.empty() && std::isspace(str.front()))
        str.remove_prefix(1);
    return str;
}

static auto remove_back_whitespace(std::string_view str) -> std::string_view {
    while (!str.empty() && std::isspace(str.back()))
        str.remove_suffix(1);
    return str;
}


static auto parse_relocation(std::string_view view) -> std::unique_ptr <ImmediateBase> {
    using enum RelImmediate::Operand;

    if (!view.ends_with(')')) throw_invalid("Unmatched parentheses");  
    view.remove_suffix(1);

    if (view.starts_with("hi(")) {
        return std::make_unique <RelImmediate> (view.substr(3), HI);
    } else if (view.starts_with("lo(")) {
        return std::make_unique <RelImmediate> (view.substr(3), LO);
    } else if (view.starts_with("pcrel_hi(")) {
        return std::make_unique <RelImmediate> (view.substr(9), PCREL_HI);
    } else if (view.starts_with("pcrel_lo(")) {
        return std::make_unique <RelImmediate> (view.substr(9), PCREL_LO);
    } else {
        throw_invalid("Invalid relocation format");
    }
}

static auto parse_integer(std::string_view view) -> std::unique_ptr <ImmediateBase> {
    if (view.starts_with('0')) {
        view.remove_prefix(1);
        if (view.empty()) return std::make_unique <IntImmediate> (0);

        int base = [&view]() {
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

static auto find_matching_parentheses(std::string_view view) -> const char * {
    std::size_t count = 0;
    for (auto &c : view) {
        if (c == '(') ++count;
        if (c == ')') {
            --count;
            if (count == 0) return &c;
        }
    }
    return nullptr;
}

static auto split_by_real(std::string_view view, const char * hint)
    -> std::pair <std::string_view, std::string_view> {
    std::size_t length = hint - view.begin();
    auto subview = view.substr(0, length);
    auto pos = std::ranges::find_if(view.substr(length), is_split_real);
    return { subview , view.substr(pos - view.begin()) };
}

static auto find_new_single(std::string_view view) {
    struct Result {
        std::string_view data;  // First part
        std::string_view rest;  // Remain part
        bool is_single;
        TreeImmediate::Operator op; // Operator
    };
    using enum TreeImmediate::Operator;

    // splitter: +, -, (, )
    const char *end = std::ranges::find_if(view, is_split_char);
    if (end == view.end())
        return Result { view, {}, true, END };

    auto [subview, remain] = split_by_real(view, end);
    if (remain.empty()) throw_invalid();

    if (remain.front() == '+')
        return Result { subview, remain, true, ADD };
    if (remain.front() == '-')
        return Result { subview, remain, true, SUB };
    if (const char *next {}; remain.front() == '('
    && bool(next = find_matching_parentheses(remain))) {
        std::size_t length = next - view.begin() + 1;

        subview = view.substr(1, length - 2);
        remain = remove_front_whitespace(view.substr(length));

        if (remain.empty())
            return Result { subview, {}, false, END };
        if (remain.front() == '+')
            return Result { subview, remain, false, ADD };
        if (remain.front() == '-')
            return Result { subview, remain, false, SUB };
    }

    throw_invalid();
}

/** May include add/sub expression. */
static auto parse_expressions(std::string_view view) -> std::unique_ptr <ImmediateBase> {
    auto pos = std::ranges::find_if(view, is_split_char);
    // Integer or label.
    if (pos == view.end()) return parse_single(view);
    // Negative integer.
    if (pos == view.begin() && *pos == '-') return parse_integer(view);

    std::vector <TreeImmediate::Pair> data;

    do {
        auto [subview, remain, is_single, op] = find_new_single(view);
        if (is_single) {
            data.emplace_back(Immediate{parse_single(subview)}, op);
        } else {
            data.emplace_back(Immediate{subview}, op);
        }
        view = remain;
        if (op == TreeImmediate::Operator::END) break;
        view = remove_front_whitespace(view.substr(1));
    } while (true);

    if (!view.empty()) throw_invalid();
    return std::make_unique <TreeImmediate> (std::move(data));
}

static auto parse_immediate(std::string_view view) -> std::unique_ptr <ImmediateBase> {
    view = remove_back_whitespace(remove_front_whitespace(view));
    if (view.empty()) throw_invalid();
    if (view.starts_with('%')) {
        return parse_relocation(view.substr(1));
    } else {
        return parse_expressions(view);
    }
}

static auto imm_to_string(ImmediateBase *imm) -> std::string {
    if (auto ptr = dynamic_cast <IntImmediate *> (imm)) {
        return std::to_string(static_cast <target_ssize_t> (ptr->data));
    } else if (auto ptr = dynamic_cast <StrImmediate *> (imm)) {
        return std::string(ptr->data.to_sv());
    } else if (auto ptr = dynamic_cast <RelImmediate *> (imm)) {
        std::string_view op;
        switch (ptr->operand) {
            using enum RelImmediate::Operand;
            case HI: op = "hi"; break;
            case LO: op = "lo"; break;
            case PCREL_HI: op = "pcrel_hi"; break;
            case PCREL_LO: op = "pcrel_lo"; break;
            default: unreachable();
        }
        std::string str = ptr->imm.to_string();
        if (str.starts_with('(') && str.ends_with(')'))
            return std::format("%{}{}", op, str);
        return std::format("%{}({})", op, str);
    } else if (auto ptr = dynamic_cast <TreeImmediate *> (imm)) {
        std::vector <std::string> vec;
        for (auto &[imm, op] : ptr->data) {
            std::string_view op_str =
                op == TreeImmediate::Operator::ADD ? " + " :
                op == TreeImmediate::Operator::SUB ? " - " : "";
            vec.push_back(std::format("{}{}", imm.to_string(), op_str));
        }
        std::size_t length {2};
        std::string string {'('};
        for (auto &str : vec) length += str.size();
        string.reserve(length);
        for (auto &str : vec) string += str;
        string += ')';
        return string;
    } else {
        unreachable();
    }
}

Immediate::Immediate(std::string_view view) : data(parse_immediate(view)) {}

std::string Immediate::to_string() const {
    return imm_to_string(data.get());
}


} // namespace dark
