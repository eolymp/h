#include "eolymp.h"

struct Limits {
    long long n_max;
};

int main(int argc, char** argv) {
    eo::validator v(argc, argv);
    auto const lim = v.subtasks<Limits>({
        {1, {10}},
        {2, {2000}},
    }).without_group({2000});

    v.feature("a leaf-heavy tree");
    int const n = v.read_int(2, lim.n_max, "n");
    v.read_eoln();
    std::vector<eo::edge> const edges = v.read_tree(n, "edge");

    std::vector<int> degree(static_cast<std::size_t>(n) + 1, 0);
    for (eo::edge const& one : edges) {
        degree[static_cast<std::size_t>(one.u)]++;
        degree[static_cast<std::size_t>(one.v)]++;
    }
    int leaves = 0;
    for (int at = 1; at <= n; at++)
        if (degree[static_cast<std::size_t>(at)] == 1) leaves++;
    if (leaves * 2 > n) v.saw("a leaf-heavy tree");
}
