# Test shapes

`eolymp-shapes.h` is the second header. It holds the shapes a generator draws from — trees,
graphs, sequences, strings and points — and the relabelling that a hand-written generator
forgets. It is opt-in: a program that does not include it pays nothing for it.

```cpp
#include <eolymp.h>
#include <eolymp-shapes.h>

int main(int argc, char** argv) {
    eo::generator g(argc, argv);
    int n = g.option<int>("n", 1, 200000);
    std::string kind = g.option<std::string>("shape", {"random", "path", "star", "caterpillar"}, "random");

    eo::graph tree = eo::shapes::tree(g.rng("tree"), n, kind);
    std::vector<long long> w = g.rng("weights").ints(n - 1, 1, 1000000000);
    std::vector<eo::edge> edges = eo::shapes::presented(g.rng("labels"), tree);

    g.out.line(n);
    for (int at = 0; at + 1 < n; at++)
        g.out.line(edges[at].u, edges[at].v, w[at]);
}
```

The generator attaches neither file. Both are in the judge's C++ runtime, beside each other
in `/usr/include/`, so including them is all it takes:

```cpp
#include <eolymp.h>
#include <eolymp-shapes.h>
```

## The rule that applies to every graph

**Relabel the vertices, turn the endpoints and shuffle the edge list before printing.** Skip
it and every tree you emit is rooted at 1 with `parent < child` in construction order, so a
wrong solution that happens to process vertices in input order passes, and an `O(n²)`
solution may never reach its worst case. The tree *is* random; its presentation is not, and
that is invisible in review.

`presented` is that step, and it is not optional advice here — it is where a shape becomes an
edge list:

```cpp
std::vector<eo::edge> edges = eo::shapes::presented(draw, made);
```

It relabels through a random permutation, swaps the endpoints of about half the edges, and
shuffles the list. A graph built by `dag` carries `directed`, and `presented` then relabels
and shuffles but leaves each arrow pointing the way it was built.

If the format is a parent array rather than an edge list, the edges cannot be shuffled, but
the construction order must still be hidden. `parent_array` relabels within the constraint
that a parent comes before its child:

```cpp
std::vector<int> parents = eo::shapes::parent_array(draw, made);
for (int at = 0; at + 1 < n; at++) g.out.line(parents[at]);
```

## Trees

| Call | Gives |
| --- | --- |
| `random_tree(draw, n)` | the three-line random *recursive* tree: depth Θ(log n), bushy |
| `uniform_tree(draw, n)` | uniform over all n^(n-2) labelled trees, through Prüfer: diameter Θ(√n) |
| `deep_tree(draw, n, lean)` | the parent drawn toward the last vertex; larger `lean` is deeper |
| `path(n)` | the bamboo: depth n, the worst case for a recursive DFS |
| `star(n)` | one vertex of degree n−1 |
| `caterpillar(draw, n, spine = 0)` | a spine with legs: a long path and a high degree at once |
| `broom(n, handle = 0)` | a path, then a star at its end |
| `binary_tree(n)`, `kary_tree(n, k)` | perfectly balanced, depth log n |
| `dumbbell(n)` | two hubs joined by an edge; at even n it has exactly two centroids |
| `spider(n, legs)` | legs of equal length from one centre |
| `tree(draw, n, name)` | any of `random`, `uniform`, `path`, `star`, `caterpillar`, `broom`, `binary`, `dumbbell` by name |

**`random_tree` is not "a random tree".** It is a random recursive tree, and its depth is
Θ(log n) — 13 at n = 1,000, 25 at n = 100,000. It will never stress a recursive DFS and never
produce a long path. It is a good default case and one shape, not the shape.

**`deep_tree` cannot give you a bamboo either.** Even `lean = 50` reaches depth about 429 out
of 100,000. If you need Θ(n) depth, that is `path`.

`tree(draw, n, name)` exists so that one generator covers several shapes and the generation
script documents the plan by itself. A name it does not know is a jury error, not a silent
default.

## Graphs

| Call | Gives |
| --- | --- |
| `connected_graph(draw, n, m)` | a connected simple graph with exactly `m` edges |
| `sparse_graph(draw, n, m)` | a simple graph with exactly `m` edges, connected or not |
| `complete_graph(n)` | the density ceiling |
| `cycle(n)`, `cycle_with_chords(draw, n, chords)` | every degree 2, then just enough cycles to break tree algorithms |
| `grid(rows, columns)` | planar, diameter √n |
| `bipartite_graph(draw, left, right, m)`, `complete_bipartite(left, right)` | only cross edges; kills odd-cycle assumptions |
| `many_components(draw, n, pieces)` | disjoint pieces; catches "assume connected" |
| `dag(draw, n, m)` | a hidden topological order, `directed` set |
| `functional(draw, n, name)` | `f(i)` for each `i`: `cycle`, `rho`, `self` or `random` |

**The edge count is exact or it is a jury error.** `connected_graph(draw, 10, 8)` says
*a connected graph on 10 vertices has 9..45 edges, not 8* rather than looping. Filling is
rejection sampling while the graph is sparse and switches to enumerate-and-shuffle once `m`
passes a quarter of `n(n−1)/2`, so a near-complete graph does not stall.

**A DAG never comes out in topological order.** `dag` draws a random order first and emits
every arrow along it, so a solution that ignores the actual sort cannot pass by accident.

## Sequences

| Call | Catches |
| --- | --- |
| `equal_values(count, value)` | solutions that assume distinct; the overflow bait `n × V` |
| `few_distinct(draw, count, kinds, low, high)` | ties everywhere |
| `plateaus(draw, count, runs, low, high)` | long runs of one value |
| `nearly_sorted(draw, count, low, high, swaps)` | sorted with `swaps` random swaps |
| `alternating(draw, count, low, high)` | "merge adjacent" heuristics |
| `hash_collisions(count, buckets = 107897)` | an `unordered_map` with the default hash |

`draw.partition(t, n)` splits a total across test cases, which is how you build the two tests
that catch different bugs: `t = 10^5` cases of `n = 1`, and one case of `n = 10^5`.

## Strings

| Call | Gives |
| --- | --- |
| `repeated('a', length)` | maximal borders; worst case for naive matching |
| `periodic(unit, length)` | long borders; tests period arithmetic |
| `near_periodic(draw, unit, length, allowed)` | exactly one mismatch from periodic |
| `fibonacci_word(length)`, `thue_morse(length)` | many distinct factors; worst cases for suffix structures |
| `palindrome(draw, length, allowed)` | worst case for Manacher and palindromic trees |

A uniform random string over a large alphabet is the easy case; `draw.letters(n,
eo::charset("ab"))` over an alphabet of two maximises repeats, borders and periods.

## Points

`eo::point` holds two `long long`s.

| Call | Gives |
| --- | --- |
| `scattered(draw, count, limit)` | random points in a box |
| `collinear(draw, count, limit)` | exactly collinear; kills cross-product sign assumptions |
| `convex_position(draw, count, limit)` | points in convex position: the turn never reverses, so none is strictly inside the hull of the others |
| `cocircular(draw, count)` | exact lattice points on one circle |
| `extreme_points(draw, count, limit)` | every point on the edge of the box, so cross products reach 10^18 |

**`convex_position` is convex, not strictly convex.** The steps are integer vectors inside a
bounded box, so many of them come out parallel and the points they build are collinear in
threes: measured at 1,000 points in ±1,000 there are 760 collinear triples, and at 100,000 in
±10^6 there are 74,726 — and never a reversed turn. Every point is on the hull's *boundary*,
but the hull has far fewer *vertices* than you asked for. If the statement promises that no
three points are collinear, this is not the generator for it.

**`cocircular` is bounded by arithmetic, not by the library.** Lattice points on a circle are
scarce: the radii this library ships top out at 972 of them, and finding more costs a scan
proportional to the radius. Asking for thousands is a jury error. Larger co-circular sets do
exist under 10^9 — r = 48,612,265 carries 2,916 — but they are not worth the scan.

## What the shapes do not do

- **They do not check the statement.** A shape that the problem forbids is still generated;
  the validator is what rejects it, and running the validator over every generated test is
  the check that matters.
- **They do not choose the plan.** Which shapes a problem admits, and which anti-test belongs
  to which wrong approach, is a question about the problem, not about this header.
- **`scattered` may repeat a point.** That is what random means. `draw.distinct` will not
  help — it draws scalars, not points. For points that must differ, draw distinct scalars in
  `0 .. (2·limit+1)² − 1` and split each into a coordinate pair, or use `convex_position`,
  `collinear` or `extreme_points`, all of which are distinct by construction.

## Reference card

| Group | Names |
| --- | --- |
| presentation | `presented`, `parent_array` |
| trees | `tree`, `random_tree`, `uniform_tree`, `deep_tree`, `path`, `star`, `caterpillar`, `broom`, `binary_tree`, `kary_tree`, `dumbbell`, `spider` |
| graphs | `connected_graph`, `sparse_graph`, `complete_graph`, `cycle`, `cycle_with_chords`, `grid`, `bipartite_graph`, `complete_bipartite`, `many_components`, `dag`, `functional` |
| sequences | `equal_values`, `few_distinct`, `plateaus`, `nearly_sorted`, `alternating`, `hash_collisions` |
| strings | `repeated`, `periodic`, `near_periodic`, `fibonacci_word`, `thue_morse`, `palindrome` |
| points | `scattered`, `collinear`, `convex_position`, `cocircular`, `extreme_points` |
| types | `eo::graph` (`n`, `edges`, `directed`), `eo::edge`, `eo::point` |

Everything lives in `eo::shapes::`, except `eo::graph`, `eo::edge` and `eo::point`.
