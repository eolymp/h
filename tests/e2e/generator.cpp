#include "../../eolymp.h"

#include <algorithm>

int main(int argc, char** argv) {
    eo::generator g(argc, argv);
    int n = g.option<int>("n", 1, 200000);
    int top = g.option<int>("max", 1, 1000000000, 1000000000);
    std::string shape = g.option<std::string>("shape", {"random", "sorted"}, "random");

    std::vector<long long> a = g.rng("values").ints(n, 1, top);
    if (shape == "sorted") std::sort(a.begin(), a.end());

    g.out.line(n);
    g.out.line(a);
}
