#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "core.h"
#include "fmt.h"

namespace eo {

struct edge {
    int u;
    int v;
};

enum graph_shape {
    any_graph = 0,
    simple = 1,
    connected = 2,
};

inline graph_shape operator|(graph_shape left, graph_shape right) {
    return static_cast<graph_shape>(static_cast<int>(left) | static_cast<int>(right));
}

class [[nodiscard]] check_result {
public:
    check_result() = default;
    check_result(std::string complaint) : complaint_(std::move(complaint)) {}

    explicit operator bool() const { return complaint_.empty(); }
    std::string const& message() const { return complaint_; }

private:
    std::string complaint_;
};

template <class T>
[[nodiscard]] inline check_result all_distinct(std::vector<T> const& values) {
    std::map<T, std::size_t> seen;
    for (std::size_t at = 0; at < values.size(); at++) {
        auto const found = seen.find(values[at]);
        if (found != seen.end())
            return check_result(fmt("elements {} and {} are both {}", found->second + 1, at + 1, values[at]));
        seen.emplace(values[at], at);
    }
    return {};
}

template <class T>
[[nodiscard]] inline check_result is_sorted(std::vector<T> const& values) {
    for (std::size_t at = 1; at < values.size(); at++)
        if (values[at] < values[at - 1])
            return check_result(
                fmt("element {} is {} and element {} is {}", at, values[at - 1], at + 1, values[at]));
    return {};
}

[[nodiscard]] inline check_result is_permutation(std::vector<int> const& values) {
    int const size = static_cast<int>(values.size());
    std::vector<int> place(static_cast<std::size_t>(size) + 1, 0);
    for (std::size_t at = 0; at < values.size(); at++) {
        int const value = values[at];
        if (value < 1 || value > size)
            return check_result(fmt("element {} is {}, outside 1..{}", at + 1, value, size));
        if (place[static_cast<std::size_t>(value)] != 0)
            return check_result(fmt("elements {} and {} are both {}", place[static_cast<std::size_t>(value)],
                                    at + 1, value));
        place[static_cast<std::size_t>(value)] = static_cast<int>(at) + 1;
    }
    return {};
}

namespace detail {

inline check_result vertices_are_inside(int n, std::vector<edge> const& edges) {
    for (std::size_t at = 0; at < edges.size(); at++) {
        edge const& one = edges[at];
        if (one.u < 1 || one.u > n || one.v < 1 || one.v > n)
            return check_result(fmt("edge {} is ({}, {}), outside 1..{}", at + 1, one.u, one.v, n));
    }
    return {};
}

inline int root_of(std::vector<int>& parent, int vertex) {
    while (parent[static_cast<std::size_t>(vertex)] != vertex) {
        parent[static_cast<std::size_t>(vertex)] = parent[static_cast<std::size_t>(
            parent[static_cast<std::size_t>(vertex)])];
        vertex = parent[static_cast<std::size_t>(vertex)];
    }
    return vertex;
}

}  // namespace detail

[[nodiscard]] inline check_result is_simple_graph(int n, std::vector<edge> const& edges) {
    if (check_result inside = detail::vertices_are_inside(n, edges); !inside) return inside;
    std::map<std::pair<int, int>, std::size_t> seen;
    for (std::size_t at = 0; at < edges.size(); at++) {
        edge const& one = edges[at];
        if (one.u == one.v) return check_result(fmt("edge {} is a loop at vertex {}", at + 1, one.u));
        std::pair<int, int> const key{std::min(one.u, one.v), std::max(one.u, one.v)};
        auto const found = seen.find(key);
        if (found != seen.end())
            return check_result(
                fmt("edges {} and {} are both ({}, {})", found->second + 1, at + 1, key.first, key.second));
        seen.emplace(key, at);
    }
    return {};
}

[[nodiscard]] inline check_result is_connected(int n, std::vector<edge> const& edges) {
    if (check_result inside = detail::vertices_are_inside(n, edges); !inside) return inside;
    std::vector<int> parent(static_cast<std::size_t>(n) + 1);
    for (int vertex = 1; vertex <= n; vertex++) parent[static_cast<std::size_t>(vertex)] = vertex;
    for (edge const& one : edges) {
        int const left = detail::root_of(parent, one.u);
        int const right = detail::root_of(parent, one.v);
        parent[static_cast<std::size_t>(left)] = right;
    }
    for (int vertex = 2; vertex <= n; vertex++)
        if (detail::root_of(parent, vertex) != detail::root_of(parent, 1))
            return check_result(fmt("vertex {} cannot be reached from vertex 1", vertex));
    return {};
}

[[nodiscard]] inline check_result is_tree(int n, std::vector<edge> const& edges) {
    if (edges.size() + 1 != static_cast<std::size_t>(n))
        return check_result(fmt("a tree on {} vertices has {} edges, not {}", n, n - 1, edges.size()));
    if (check_result simple_enough = is_simple_graph(n, edges); !simple_enough) return simple_enough;
    return is_connected(n, edges);
}

}  // namespace eo
