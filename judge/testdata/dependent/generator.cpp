#include "eolymp.h"

int main(int argc, char** argv) {
    eo::generator g(argc, argv);
    int const n = g.option<int>("n", 1, 1000);
    int const m = g.option<int>("m", 1, 1000);
    g.require(m <= n, "-m={} is more than -n={}", m, n);
    g.out.line(n, m);
}
