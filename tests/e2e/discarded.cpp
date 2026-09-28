#include "../../eolymp.h"

void spend(eo::interactor& it) { eo::budget(it, 20, "queries"); }

int main(int argc, char** argv) {
    eo::validator v(argc, argv);
    eo::allow("EO106", "a bare statement ends at its semicolon");
    int const t = v.read_int(1, 200001, "t");
    v.read_eoln();
    eo::sum_limit(10, "sum of n");
    (void)t;
}
