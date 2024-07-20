#include <utility.h>
#include <config/config.h>
#include <interpreter/interpreter.h>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <fmtlib.h>


int main(int argc, char** argv) {
    using std::chrono::high_resolution_clock;
    using ms = std::chrono::milliseconds;

    // try {
        [[maybe_unused]]
        auto start_time = high_resolution_clock::now();
        auto config_ptr = dark::Config::parse(argc, argv);
        const auto& config = *config_ptr;

        dark::Interpreter interpreter(config);

        interpreter.assemble();
        interpreter.link();

        [[maybe_unused]]
        auto build_time = high_resolution_clock::now();
        if (!config.has_option("silent")) {
            auto build_str = std::format(" Build time: {}ms ",
                duration_cast<ms>(build_time - start_time).count());
            std::cout << std::format("\n{:=^80}\n\n", build_str);
        }

        interpreter.simulate();

        [[maybe_unused]]
        auto interpret_time = high_resolution_clock::now();
        if (!config.has_option("silent")) {
            auto interpret_str = std::format(" Interpret time: {}ms ",
                duration_cast<ms>(interpret_time - build_time).count());
            std::cout << std::format("\n{:=^80}\n\n", interpret_str);
        }
    // }
    //  catch (std::exception& e) {
    //     std::cerr << std::format("std::exception caught: {}\n", e.what());
    //     dark::unreachable();
    // } catch (...) {
    //     std::cerr << "unexpected exception caught\n";
    //     dark::unreachable();
    // }
    return 0;
}
