#include <config/config.h>
#include <interpreter/interpreter.h>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <fmtlib>

int main(int argc, char** argv) {
    using std::chrono::high_resolution_clock;
    using ms = std::chrono::milliseconds;

    auto start_time = high_resolution_clock::now();
    auto config_ptr = dark::Config::parse(argc, argv);
    const auto& config = *config_ptr;

    dark::Interpreter interpreter(config);
    auto build_time = high_resolution_clock::now();
    auto build_str = std::format(" Build time: {}ms ",
        duration_cast<ms>(build_time - start_time).count());
    std::cout << std::format("\n{:=^80}\n\n", build_str);

    interpreter.interpret(config);

    auto interpret_time = high_resolution_clock::now();
    auto interpret_str = std::format(" Interpret time: {}ms ",
        duration_cast<ms>(interpret_time - build_time).count());
    std::cout << std::format("\n{:=^80}\n\n", interpret_str);
    return 0;
}
