#include "assembly/immediate.h"
#include "assembly/exception.h"
#include "assembly/frontend/match.h"
#include "assembly/frontend/parser.h"
#include "utility/cast.h"
#include "utility/error.h"
#include <unordered_map>

/* Some static functions. */
namespace dark::frontend {

using Owner_t = std::unique_ptr<ImmediateBase>;

static auto parse_integer(std::string_view view) -> Owner_t {
    if (view.starts_with('0')) {
        view.remove_prefix(1);
        if (view.empty())
            return std::make_unique<IntImmediate>(0);

        int base = [&view]() {
            if (view.starts_with('x'))
                return view.remove_prefix(1), 16;
            if (view.starts_with('b'))
                return view.remove_prefix(1), 2;
            /* No choice */ return 8;
        }();

        auto number = sv_to_integer<target_size_t>(view, base);
        throw_if(!number.has_value(), "Invalid integer format");

        return std::make_unique<IntImmediate>(*number);
    } else {
        // The default value_or is invalid.
        auto number = sv_to_integer<target_size_t>(view, 10);

        // Allow to be negative, but the value should be in range.
        throw_if(!number.has_value(), "integer out of range");

        return std::make_unique<IntImmediate>(*number);
    }
}

static auto parse_negative(std::string_view view) -> Owner_t {
    auto imm           = parse_integer(view);
    auto &value        = static_cast<IntImmediate *>(imm.get())->data;
    constexpr auto max = target_size_t(std::numeric_limits<target_ssize_t>::max()) + 1;
    throw_if(value > max, "integer out of range");
    value = -value;
    return imm;
}

static auto parse_identifier(std::string_view view) -> Owner_t {
    throw_if(view.empty(), "Invalid immediate");
    if (std::isdigit(view.front())) {
        return parse_integer(view);
    } else { // Label case.
        return std::make_unique<StrImmediate>(view);
    }
}

static auto parse_character(std::string_view view) -> Owner_t {
    throw_if(!view.starts_with('\'') || !view.ends_with('\''), "Invalid character");
    if (view.size() == 3 && view[1] != '\\') {
        return std::make_unique<IntImmediate>(view[1]);
    } else if (view.size() == 4 && view[1] == '\\') {
        switch (view[2]) {
            case 'n':  return std::make_unique<IntImmediate>('\n');
            case 't':  return std::make_unique<IntImmediate>('\t');
            case 'r':  return std::make_unique<IntImmediate>('\r');
            case '0':  return std::make_unique<IntImmediate>('\0');
            case '\\': return std::make_unique<IntImmediate>('\\');
            case '\'': return std::make_unique<IntImmediate>('\'');
            default:   throw FailToParse("Invalid character");
        }
    }
    throw FailToParse("Invalid character");
}

} // namespace dark::frontend

/* The implementation of ImmediateParser */
namespace dark::frontend {

auto ImmediateParser::find_right_parenthesis(TokenStream &stream) const -> TokenStream {
    using enum Token::Type;

    throw_if(!match<Parenthesis, Placeholder>(stream));
    throw_if(stream[0].what != "(", "Invalid immediate");

    auto length = this->matched.at(&stream[0]);
    runtime_assert(length < stream.size());

    // Note: stream[length] = ')'.
    return stream.split_at(length + 1).subspan(1, length - 1);
}

ImmediateParser::ImmediateParser(TokenStream tokens) {
    std::vector<const Token *> stack;

    for (auto &token : tokens) {
        if (token.what == "(") {
            stack.push_back(&token);
        } else if (token.what == ")") {
            throw_if(stack.empty(), "Unmatched right parenthesis");
            auto prev           = stack.back();
            this->matched[prev] = &token - prev;
            stack.pop_back();
        }
    }

    throw_if(!stack.empty(), "Unmatched left parenthesis");
}

auto ImmediateParser::parse(TokenStream tokens) const -> Owner_t {
    using enum TreeImmediate::Operator;

    throw_if(tokens.empty(), "Invalid immediate");
    using Tree_t = decltype(TreeImmediate::data);
    Tree_t tree;

    if (tokens[0].type == Token::Type::Operator) {
        throw_if(tokens[0].what != "-", "unsupported operator {}", tokens[0].what);
        tree.emplace_back(Immediate{0}, SUB);
        tokens = tokens.subspan(1);
        throw_if(tokens.empty(), "Invalid immediate");
    }

    do {
        auto imm = Immediate{this->find_single_op(tokens)};
        if (tokens.empty()) {
            tree.emplace_back(std::move(imm), END);
            return std::make_unique<TreeImmediate>(std::move(tree));
        } else if (tokens[0].what == "+") {
            tree.emplace_back(std::move(imm), ADD);
        } else if (tokens[0].what == "-") {
            tree.emplace_back(std::move(imm), SUB);
        } else {
            throw FailToParse("Invalid immediate");
        }
        tokens = tokens.subspan(1); // Remove the operator.
    } while (true);
}

auto ImmediateParser::find_single_op(TokenStream &tokens) const -> Owner_t {
    throw_if(tokens.empty(), "Invalid immediate");
    using enum Token::Type;
    auto token = tokens[0];
    switch (token.type) {
        case Identifier: tokens = tokens.subspan(1); return parse_identifier(token.what);

        case Operator:
            throw_if(token.what != "-", "Invalid immediate");
            tokens = tokens.subspan(1);
            throw_if(tokens.empty(), "Invalid immediate");
            throw_if(tokens[0].type != Identifier, "Invalid immediate");
            token  = tokens[0];
            tokens = tokens.subspan(1);
            return parse_negative(token.what);

        case Character:   tokens = tokens.subspan(1); return parse_character(token.what);

        case Parenthesis: return parse(find_right_parenthesis(tokens));

        case Relocation:  {
            tokens = tokens.subspan(1);

            auto inner = Immediate{parse(find_right_parenthesis(tokens))};
            auto view  = token.what.substr(1); // skip the '%'

            using enum RelImmediate::Operand;
            if (view.starts_with("hi")) {
                return std::make_unique<RelImmediate>(std::move(inner), HI);
            } else if (view.starts_with("lo")) {
                return std::make_unique<RelImmediate>(std::move(inner), LO);
            } else if (view.starts_with("pcrel_hi")) {
                return std::make_unique<RelImmediate>(std::move(inner), PCREL_HI);
            } else if (view.starts_with("pcrel_lo")) {
                return std::make_unique<RelImmediate>(std::move(inner), PCREL_LO);
            } else {
                throw FailToParse("Invalid relocation format");
            }
        }

        default: throw FailToParse("Invalid immediate");
    }
}

} // namespace dark::frontend

namespace dark {

Immediate::Immediate(target_size_t data) : data(std::make_unique<IntImmediate>(data)) {}

} // namespace dark
