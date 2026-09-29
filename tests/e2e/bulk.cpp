#include "../../eolymp.h"

int main(int argc, char** argv) {
    eo::controller ctl(argc, argv);
    int const n = ctl.input.read_int(1, 1000000, "n");
    eo::channel& only = ctl.spawn();
    only.send(n);
    for (int at = 1; at <= n; at++) only.send(at);
    long long total = 0;
    for (int at = 1; at <= n; at++) total += only.read_long(0, 2000000, "double");
    long long const expected = 1LL * n * (n + 1);
    if (total != expected) eo::wrong("the doubles add up to {}, expected {}", total, expected);
    eo::accept("{} lines sent before the first answer was read", n);
}
