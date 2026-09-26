#include "../../eolymp-shapes.h"

#include <cstdio>

int main() {
    eo::rng draw(20260922);

    for (eo::point const& one : eo::shapes::convex_position(draw, 500, 100000))
        std::printf("c %lld %lld\n", one.x, one.y);
    for (eo::point const& one : eo::shapes::cocircular(draw, 100)) std::printf("o %lld %lld\n", one.x, one.y);
    for (eo::point const& one : eo::shapes::collinear(draw, 100, 1000000))
        std::printf("l %lld %lld\n", one.x, one.y);
    for (eo::point const& one : eo::shapes::extreme_points(draw, 100, 1000000))
        std::printf("e %lld %lld\n", one.x, one.y);

    for (long long const one : draw.distinct(100, 1, 1000000000)) std::printf("d %lld\n", one);
    for (long long const one : draw.partition(20, 1000)) std::printf("p %lld\n", one);

    eo::graph const made = eo::shapes::connected_graph(draw, 200, 600);
    for (eo::edge const& one : eo::shapes::presented(draw, made)) std::printf("g %d %d\n", one.u, one.v);
    for (int const one : eo::shapes::parent_array(draw, eo::shapes::uniform_tree(draw, 200)))
        std::printf("t %d\n", one);
}
