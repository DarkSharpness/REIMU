#include "utility/error.h"
#include <iostream>

namespace dark::console {

std::ostream error{std::cerr.rdbuf()};
std::ostream warning{std::cerr.rdbuf()};
std::ostream message{std::cout.rdbuf()};
std::ostream profile{std::cerr.rdbuf()};

/**
 * @brief Flushes the standard output stream.
 * Used to synchronize std::cout with std::cerr.
 */
void flush_stdout() {
    std::cout.flush();
}

} // namespace dark::console
