#include "assembly/assembly.h"
#include "assembly/forward.h"
#include "assembly/frontend/lexer.h"
#include "assembly/frontend/match.h"
#include "fmtlib.h"
#include "utility/error.h"
#include <algorithm>
#include <fstream>
#include <memory>
#include <string_view>

namespace dark {

/* Whether the character is a valid token character. */
bool is_label_char(char c) {
    return std::isalnum(c) || c == '_' || c == '.' || c == '@' || c == '$';
}

static auto to_shared(std::string_view str) -> std::shared_ptr<char[]> {
    auto ptr = std::make_shared<char[]>(str.size() + 1);
    std::ranges::copy(str, ptr.get());
    ptr[str.size()] = '\0';
    return ptr;
}

Assembler::Assembler(std::string_view file_name_) :
    current_section(Section::UNKNOWN), file_name(file_name_), sp(to_shared(file_name_)),
    line_number(0) {
    std::ifstream file{this->file_name};
    panic_if(!file.is_open(), "Failed to open {}", this->file_name);

    this->line_number = 0;
    std::string line; // Current line

    while (std::getline(file, line)) {
        ++this->line_number;
        try {
            this->parse_line(line);
        } catch (FailToParse &e) {
            file.close();
            handle_build_failure(
                fmt::format("Fail to parse source assembly.\n {}\n", e.inner), this->file_name,
                this->line_number
            );
        } catch (std::exception &e) {
            unreachable(fmt::format("Unexpected error: {}\n", e.what()));
        }
    }
}

using frontend::Lexer;
using frontend::Token;
using enum Token::Type;
using frontend::match;

/**
 * @brief Parse a line of the assembly.
 * @param line Line of the assembly.
 */
void Assembler::parse_line(std::string_view line) {
    Lexer lexer{line};
    const auto tokens = lexer.get_stream();

    if (tokens.empty())
        return; // Empty line

    throw_if(
        tokens[0].type != Token::Type::Identifier,
        "Expected a label or command or storage, got \"{}\"", tokens[0].what
    );

    // Label + colon case.
    if (match<Identifier, Colon>(tokens))
        return this->add_label(tokens[0].what);

    const auto rest = tokens.subspan(1);

    if (tokens[0].what.starts_with('.')) {
        return this->parse_storage_new(tokens[0].what, rest);
    } else {
        return this->parse_command_new(tokens[0].what, rest);
    }
}

} // namespace dark
