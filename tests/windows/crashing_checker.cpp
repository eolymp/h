#include "../../eolymp.h"

#include <cstdlib>

int main(int argc, char** argv) {
    eo::checker c(argc, argv);
    int const n = c.input.read_int("n");
    for (int line = 1; line <= n; line++) eo::log("line {} of the log", line);
    std::abort();
}
