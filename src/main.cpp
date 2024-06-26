#include "config.h"
#include "interpreter/interpreter.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <fmtlib>

int main(int argc, char** argv) {
    auto start_time = std::chrono::high_resolution_clock::now();
    const auto config = dark::Config::parse(argc, argv);
    dark::Interpreter interpreter(config);
    auto parse_time = std::chrono::high_resolution_clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(parse_time - start_time);
    std::cerr << std::format("\nParsing time: {}ms\n", delta.count());
    return 0;
}
