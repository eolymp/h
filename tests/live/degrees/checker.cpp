#include "eolymp.h"

int main(int argc, char** argv) {
    eo::checker c(argc, argv);
    c.answers(eo::unique);

    int const n = c.input.read_int(2, 2000, "n");
    int matched = 0;
    for (int at = 1; at <= n; at++) {
        int const wanted = c.jury.read_int(1, n - 1, eo::unnamed);
        int const said = c.output.read_int(0, n - 1, "degree");
        if (said == wanted) matched++;
    }
    if (matched == n) eo::accept("all {} degrees", n);
    eo::score(static_cast<double>(matched) / n, "{} of {} degrees", matched, n);
}
