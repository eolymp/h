#include "eolymp.h"

int main(int argc, char** argv) {
    eo::validator v(argc, argv);
    int const n = v.read_int(1, 1000, "n");
    v.read_space();
    v.read_int(1, n, "m");
    v.read_eoln();
}
