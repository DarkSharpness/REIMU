#include "config/config.h"
#include "fmtlib.h"
#include "interpreter/interpreter.h"
#include "utility/error.h"
#include <chrono>

int main(int argc, char **argv) {
    using std::chrono::high_resolution_clock;
    using ms = std::chrono::milliseconds;
    using dark::console::message;

    try {
        auto start_time    = high_resolution_clock::now();
        auto config_ptr    = dark::Config::parse(argc, argv);
        const auto &config = *config_ptr;

        dark::Interpreter interpreter(config);

        interpreter.assemble();
        interpreter.link();

        auto build_time = high_resolution_clock::now();
        auto build_str =
            fmt::format(" Build time: {}ms ", duration_cast<ms>(build_time - start_time).count());
        message << fmt::format("\n{:=^80}\n\n", build_str);

        interpreter.simulate();

        auto interpret_time = high_resolution_clock::now();
        auto interpret_str  = fmt::format(
            " Interpret time: {}ms ", duration_cast<ms>(interpret_time - build_time).count()
        );
        message << fmt::format("\n{:=^80}\n\n", interpret_str);
    } catch (dark::PanicError &e) { return 1; } catch (std::exception &e) {
        dark::unreachable(fmt::format("std::exception caught: {}\n", e.what()));
    } catch (...) { dark::unreachable("unexpected exception caught\n"); }
    return 0;
}
