# Coming from testlib

Most jury programs written today use [testlib](https://github.com/MikeMirzayanov/testlib).
This page puts each testlib call next to what eolymp.h does instead, role by role, and then
lists the differences that change a program's behaviour rather than its spelling. Every name
in the right-hand columns is in the header; the programs below are compiled by `make check`.

## Validator

| testlib | eolymp.h |
| --- | --- |
| `registerValidation(argc, argv);` | `eo::validator v(argc, argv);` |
| `inf.readInt(1, n, "n")` | `v.read_int(1, n, "n")` |
| `inf.readLong(lo, hi, "x")` | `v.read_long(lo, hi, "x")` |
| `inf.readStrictDouble(lo, hi, 1, 6, "x")` | `v.read_real(lo, hi, 1, 6, "x")` |
| `inf.readToken("[a-z]{1,10}", "s")` | `v.read_token(1, 10, eo::charset("a-z"), "s")` |
| `inf.readLine("[a-z ]{1,100}", "s")` | `v.read_line(1, 100, eo::charset("a-z "), "s")` |
| `inf.readInts(n, 1, 1000000000, "a")` | `v.read_ints(n, 1, 1000000000, "a")` |
| `inf.readSpace()`, `inf.readEoln()`, `inf.readEof()` | `v.read_space()`, `v.read_eoln()`, `v.read_eof()` |
| `inf.readChar(':')` | `v.read_char(':')` |
| `inf.eoln()`, `inf.eof()` | `v.at_eoln()`, `v.at_eof()` |
| `ensuref(cond, "n is %d", n)` | `v.require(cond, "n is {}", n)` |
| `format("a[%d]", i)` as a name | `eo::element("a", i)` |
| `validator.group()` | `v.group()`, a `std::optional<int>` |

```cpp
#include <eolymp.h>

int main(int argc, char** argv) {
    eo::validator v(argc, argv);
    int n = v.read_int(1, 200000, "n");
    v.read_eoln();
    std::vector<int> a = v.read_ints(n, 1, 1000000000, "a");
    v.read_eoln();
    v.require(eo::all_distinct(a), "a");
}
```

A pattern such as `[a-z]{1,10}` becomes a charset and a length; there is no regular
expression syntax. `v.read_eof()` is optional, because the library checks the end of the
input itself. See [validator.md](validator.md).

## Checker

| testlib | eolymp.h |
| --- | --- |
| `registerTestlibCmd(argc, argv);` | `eo::checker c(argc, argv);` |
| `inf`, `ouf`, `ans` | `c.input`, `c.output`, `c.jury` |
| `ouf.readInt(1, n, "k")` | `c.output.read_int(1, n, "k")` |
| `ans.readLong()` | `c.jury.read_long(eo::any, "sum")`: bounds, or `eo::any` to say there are none |
| `quitf(_ok, "...")` | `eo::accept("...")` |
| `quitf(_wa, "got %d", x)`, `quitf(_pe, ...)` | `eo::wrong("got {}", x)`; Eolymp has no presentation error |
| `quitf(_fail, ...)` | `eo::jury_error(...)` |
| `ouf.quitf(_wa, ...)` | `c.output.wrong(...)`, blamed on that stream |
| `quitp(p)` | `eo::score(f)` for a fraction of the test, or `eo::points(p)` for points |
| a `readAns(ouf)` / `readAns(ans)` pair, then `quitf(_fail)` if the contestant beats the jury | `c.read_both(reader)` and `c.optimum(by_the_jury, found, eo::minimize)` |
| `doubleCompare(expected, result, 1e-9)` | `eo::close_enough(expected, found, 1e-9)` |
| `wcmp` | `c.tokens()` |
| `rcmp6`, `rcmp9` | `c.reals(1e-6)`, `c.reals(1e-9)` |

```cpp
#include <eolymp.h>

int main(int argc, char** argv) {
    eo::checker c(argc, argv);
    int n = c.input.read_int(1, 100000, "n");
    auto chosen = [n](eo::answer_stream& a) {
        int k = a.read_int(0, n, "k");
        std::vector<int> items = a.read_ints(k, 1, n, "item");
        if (!eo::all_distinct(items)) a.wrong("an item is chosen twice");
        return k;
    };
    c.answers(eo::many);
    auto [by_the_jury, found] = c.read_both(chosen);
    c.optimum(by_the_jury, found, eo::maximize);
}
```

`c.tokens()` compares tokens as text, as `wcmp` does; a checker that compares numbers, as
`ncmp` does, reads them with `read_long`. Nothing needs `ouf.seekEof()`: text after the
answer is a wrong answer unless the stream says `trailing(eo::ignore)`. See
[checker.md](checker.md).

## Interactor

| testlib | eolymp.h |
| --- | --- |
| `registerInteraction(argc, argv);` | `eo::interactor it(argc, argv);` |
| `inf`, `ans`, `ouf` | `it.input`, `it.jury`, `it.contestant` |
| `cout << x << endl;` | `it.send(x);`, flushed before the next read |
| `tout << ...` for the checker, which reads it back | `eo::accept`, `eo::score` or `eo::points` write the summary, and the checker is `eo::checker(argc, argv).from_interactor();` |
| `quitf(_wa, ...)` | `eo::wrong(...)` |

See [interactor.md](interactor.md).

## Generator

| testlib | eolymp.h |
| --- | --- |
| `registerGen(argc, argv, 1);` | `eo::generator g(argc, argv);` |
| `opt<int>("n")` | `g.option<int>("n", 1, 200000)`, with its range |
| `opt<int>("n", 10)` | `g.option<int>("n", 1, 200000, 10)` |
| `atoi(argv[1])`, `opt<int>(1)` | no positional arguments: `-n=10` |
| `rnd.next(a, b)` | `r.uniform(a, b)` |
| `rnd.next(n)` | `r.uniform(0, n - 1)` |
| `rnd.next(0.0, 1.0)` | `r.real(0.0, 1.0)` |
| `rnd.wnext(a, b, t)` | `r.weighted(a, b, t)` |
| `rnd.any(v)` | `r.pick(v)` |
| `shuffle(v.begin(), v.end())` | `r.shuffle(v)` |
| `rnd.perm(n, 1)` | `r.perm(n, 1)` |
| `rnd.distinct(k, a, b)` | `r.distinct(k, a, b)` |
| `rnd.partition(k, sum, least)` | `r.partition(k, sum, least)` |
| `rnd.next("[a-z]{5}")` | `r.letters(5, eo::charset("a-z"))` |
| `println(a)`, `cout << a` | `g.out.line(a)` |

`r` is `g.rng()`, the default stream, or `g.rng("label")`, a named one.

```cpp
#include <eolymp.h>

int main(int argc, char** argv) {
    eo::generator g(argc, argv);
    int n = g.option<int>("n", 1, 200000);
    int top = g.option<int>("max", 1, 1000000000, 1000000000);
    eo::rng& r = g.rng("values");
    std::vector<long long> a = r.ints(n, 1, top);
    r.shuffle(a);
    g.out.line(n);
    g.out.line(a);
}
```

See [generator.md](generator.md).

## What behaves differently

- **A generator's arguments are all `-name=value`.** testlib accepts `-n 10`, `--n=10` and
  positional arguments read with `argv[1]` or `opt<int>(1)`; eolymp.h refuses each of them
  and every option it was not told about, before writing anything. A problem moved from
  testlib therefore needs its stored generator arguments rewritten, `gen 10 3` becoming
  `gen -n=10 -k=3`. The one positional argument allowed is the seed a stress run appends.
- **The same seed does not give the same numbers.** eolymp.h draws with its own documented
  algorithm, so a test regenerated after the move is a different test of the same shape.
  `r.weighted(a, b, t)` has the distribution of `rnd.wnext(a, b, t)`, the largest of
  `t + 1` draws, but not its values.
- **A read has bounds and a name, or says it has none.** `readInt()` with neither is legal in
  testlib; in eolymp.h it compiles, with warnings EO101 and EO102 (EO103 in a checker), and
  `eo::any` or `eo::unnamed` says that is deliberate.
- **A score is a fraction of the test.** `eo::score(0.5)` on a test worth 10 pays 5 points;
  `eo::points(5)` says the same in points. A score of 2 or more is clamped to full marks and
  called a likely percentage (EO205).
- **A jury answer is read strictly by the checker too.** A malformed answer file is a jury
  error, not a wrong answer, and `c.read_both` holds the jury's answer to the same checks as
  the contestant's.
