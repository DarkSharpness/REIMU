#pragma once
#include <span>
#include <string_view>

/* Some weight configurations. */
namespace dark::weight {

static constexpr std::size_t kArith     = 1;
static constexpr std::size_t kBitwise   = 1;
static constexpr std::size_t kBranch    = 10;
static constexpr std::size_t kMemory    = 64;
static constexpr std::size_t kMultiply  = 4;
static constexpr std::size_t kDivide    = 20;
static constexpr std::size_t kManual    = 0; // Magic number

static constexpr std::string_view arith_name_list[] = {
    "add", "sub", "lui", "slt", "sltu", "auipc",
};
static constexpr std::string_view bitwise_name_list[] = {
    "and", "or", "xor", "sll", "srl", "sra",
};
static constexpr std::string_view branch_name_list[] = {
    "beq", "bne", "blt", "bge", "bltu", "bgeu",
};
static constexpr std::string_view memory_name_list[] = {
    "lb", "lh", "lw", "lbu", "lhu", "sb", "sh", "sw",
};
static constexpr std::string_view multiply_name_list[] = {
    "mul", "mulh", "mulhsu", "mulhu",
};
static constexpr std::string_view divide_name_list[] = {
    "div", "divu", "rem", "remu",
};
static constexpr std::string_view jump_name_list[] = {
    "jal = 1", "jalr = 2",
};

struct Weight_Range {
    using _List_t = std::span <const std::string_view>;
    std::string_view name;
    _List_t list;
    std::size_t weight;
    consteval Weight_Range(std::string_view name, _List_t list, std::size_t weight)
        : name(name), list(list), weight(weight) {}
};

static constexpr Weight_Range kWeightRanges[] = {
    { "arith"   , arith_name_list   , kArith    },
    { "bitwise" , bitwise_name_list , kBitwise  },
    { "branch"  , branch_name_list  , kBranch   },
    { "memory"  , memory_name_list  , kMemory   },
    { "multiply", multiply_name_list, kMultiply },
    { "divide"  , divide_name_list  , kDivide   },
    { "jump"    , jump_name_list    , kManual   },
};

constexpr auto parse_manual(std::string_view name) {
    auto pos = name.find('=');

    auto prev = name.substr(0, pos);
    auto post = name.substr(pos + 1);

    while (prev.ends_with(' '))     prev.remove_suffix(1);
    while (post.starts_with(' '))   post.remove_prefix(1);

    // Parse an integer
    std::size_t weight = 0;
    for (auto c : post) {
        if (c < '0' || c > '9') throw;
        weight = weight * 10 + (c - '0');
    }

    return std::make_pair(prev, weight);
}

static constexpr std::size_t kWeightCount = []() {
    std::size_t total_weight = 0;
    for (const auto &[name, list, weight] : kWeightRanges)
        total_weight += list.size();
    return total_weight;
} ();

static_assert(parse_manual("demo = 1") == std::pair(std::string_view("demo"), std::size_t(1)));


} // namespace dark::weight
