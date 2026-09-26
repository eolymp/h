#include "eolymp.h"
#include "eolymp-shapes.h"

int main(int argc, char** argv) {
    eo::generator g(argc, argv);
    int n = g.option<int>("n", 2, 2000);
    std::string kind = g.option<std::string>(
        "shape", {"random", "uniform", "path", "star", "caterpillar", "binary", "dumbbell"}, "random");

    eo::graph made = eo::shapes::tree(g.rng("tree"), n, kind);
    std::vector<eo::edge> shown = eo::shapes::presented(g.rng("labels"), made);

    g.out.line(n);
    for (eo::edge const& one : shown) g.out.line(one.u, one.v);
}
