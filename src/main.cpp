#include "config.h"
#include "interpreter/interpreter.h"
#include <iostream>
#include <algorithm>

int main(int argc, char** argv) {
    const auto config = dark::Config::parse(argc, argv);
    dark::Interpreter interpreter(config);
    return 0;
}
