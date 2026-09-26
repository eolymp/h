#include "../../eolymp.h"

int main(int argc, char** argv) {
    eo::validator v(argc, argv);
    int const n = v.read_int(1, 200000, "n");
    v.read_eoln();
    std::vector<eo::edge> edges;
    for (int at = 0; at + 1 < n; at++) {
        int const u = v.read_int(1, n, "u");
        v.read_space();
        int const w = v.read_int(1, n, "v");
        v.read_space();
        v.read_int(1, 1000000000, "w");
        v.read_eoln();
        edges.push_back(eo::edge{u, w});
    }
    v.require(eo::is_tree(n, edges), "edges");
}
