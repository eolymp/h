#include "../../eolymp-shapes.h"

int main(int argc, char** argv) {
    eo::generator g(argc, argv);
    int n = g.option<int>("n", 1, 200000);
    std::string kind = g.option<std::string>("shape", {"random", "path", "star", "caterpillar"}, "random");
    int top = g.option<int>("maxw", 1, 1000000000, 1000000000);

    eo::graph made = eo::shapes::tree(g.rng("tree"), n, kind);
    std::vector<long long> weights = g.rng("weights").ints(n - 1, 1, top);
    std::vector<eo::edge> shown = eo::shapes::presented(g.rng("labels"), made);

    g.out.line(n);
    for (int at = 0; at + 1 < n; at++)
        g.out.line(shown[static_cast<std::size_t>(at)].u, shown[static_cast<std::size_t>(at)].v,
                   weights[static_cast<std::size_t>(at)]);
}
