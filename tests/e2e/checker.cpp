#include "../../eolymp.h"

#include <cstdio>

int main(int argc, char** argv) {
    eo::checker c(argc, argv);
    std::printf("a line that ends in a space \n");
    std::printf("\n");
    int const n = c.input.read_int(1, 100, "n");
    std::vector<long long> const want = c.jury.read_longs(n, eo::any, "a");
    std::vector<long long> const got = c.output.read_longs(n, eo::any, "a");
    int good = 0;
    for (int at = 0; at < n; at++) good += got[at] == want[at];
    if (good == 0) eo::wrong("none of the {} answers is right", n);
    eo::score(eo::ratio(good, n), "{} of {} answers are correct", good, n);
}
