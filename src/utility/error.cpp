#include <utility/error.h>
#include <iostream>

namespace dark::console {

std::ostream error   { std::cerr.rdbuf() };
std::ostream warning { std::cerr.rdbuf() };
std::ostream message { std::cout.rdbuf() };
std::ostream program_info { std::cout.rdbuf() };

} // namespace dark::console
