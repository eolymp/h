#include "../../eolymp.h"

struct limits {
    int n;
    int value;
};

int main(int argc, char** argv) {
    eo::validator v(argc, argv);
    limits const allowed = v.subtasks<limits>({{0, {5, 10}}, {1, {200000, 1000000000}}})
                               .without_group({200000, 1000000000});
    int const n = v.read_int(1, allowed.n, "n");
    v.read_eoln();
    std::vector<int> const a = v.read_ints(n, 1, allowed.value, "a");
    v.read_eoln();
    v.require(eo::all_distinct(a), "a");
    return 0;
}
