# Changelog

## 2.0.0

This release changes verdicts, so it is a major version. Every verdict change moves a run
toward the result it should have had: a broken checker's accept becomes a jury error, and a
correct solution's wrong answer becomes an accept. The judge's C++ runtime carries one release
of the header, so rebuilding the runtime moves every problem at once. The list below is
complete, so a problem author can check whether any of their problems depends on the old
behaviour.

### Checkers and scoring

- An exception that leaves a checker, interactor or controller and is caught outside the role
  object: was an accept, or whatever code the handler chose, → a jury error. Affects a checker
  wrapped in a `try` whose `catch` is outside the role object.
- A NaN score or NaN points, including through `pass()`: was `points nan`, which the judge paid
  in full, → a jury error. Affects a checker with a 0/0 formula.
- ±inf in an interactor or controller (`eo::points(+inf)`, a score of ±inf): was a jury error
  at the checker → accept or `points 0`, as a checker already gave. Checkers are unchanged.
- Negative `eo::points`: was `points -5` → `points 0`, still exit 7.
- `eo::points` above the cost in an interactor or controller: was a jury error, a summary
  fraction above 1, → accept, as in a checker.
- A fraction above 1: unchanged, still clamped to 1; EO205 now calls 2 or more a likely
  percentage.

### Reading contestant output

- A lenient number longer than 4096 characters: was split into two values, or accepted with
  its tail unread, → a wrong answer.
- `reals()`: a numeric token longer than 4096 characters and than the answer's could be
  accepted → a wrong answer.
- A negative zero on a checker's, interactor's or controller's streams (`-0.000000`, `-0`,
  `-1e-400` in `read_real` and `reals()`): wrong answer → accept. Integers and validators are
  unchanged.
- `lines()`: a line that differs from the answer only by trailing carriage returns: wrong
  answer → accept.
- A huge contestant token or line in `tokens()`, `reals()`, `lines()` or `yes_no()`: a system
  failure from running out of memory → a wrong answer.

### Validators, generators, summaries and handoffs

- A `sum_limit` total that overflows a `long long`: the invalid test was accepted → refused.
- A generator whose write to stdout fails, on a full disk or a closed descriptor: exit 0 with
  a truncated test → exit 3.
- A summary or handoff that cannot be written whole: could pay a truncated `fraction 1` → a
  jury error.
- A summary that carries a field twice: the last value won → a jury error.
- A truncated phase handoff, or one with a negative size: SIGABRT → a jury error.
- `partition(1, s, s)` and similar in a generator: exit 3 → the test is written.

### Messages and interaction

- A message whose `{}` do not match its values, such as `eo::wrong("expected %lld", a)` or
  contestant text used as the pattern: a jury error → the intended verdict, with warning EO112
  (still fatal under `EOLYMP_STRICT`).
- A solution that exits before the interactor's or controller's last line: a wrong answer
  depending on a race → decided by what the solution said.
- The last output to a slow reader: what did not fit in the pipe was dropped, so a correct
  solution got a runtime error or a wrong answer, → delivered, with a 500 ms no-progress and a
  2 s overall cutoff.
- An interactor that sends more than 64 KB before reading, to a solution that answers as it
  reads: a deadlock, a time limit or an idleness verdict, → accept, taking in up to 16 MB of
  answers meanwhile; beyond that, warning EO409 and the old deadlock.
- An inherited non-blocking pipe: a jury error, "Resource temporarily unavailable", → the real
  verdict.
- A controller with many instances: about 1 GB per 1000 instances, which could reach the
  memory limit, → about 68 MB.
- A checker when `/tmp` is read-only or full: a jury error on every test → judges, keeping its
  output in a memfd or the working directory instead.

### Not verdicts, but visible changes

- `eo::allow(...)`, `eo::sum_limit(...)` or `eo::budget(...)` written as a bare statement now
  gets a `[[nodiscard]]` warning under GCC 10 or later and clang, an error with `-Werror`.
- `rng::real` in builds that fuse multiply-add (clang with FMA, arm64 Macs, GCC with
  `-march=native` or `haswell`, GCC on arm64) now draws the same bits as g++ on x86, so tests
  those builds generated before come out different. GCC on x86 without FMA, the judge's
  build, is unchanged.
- New or changed warnings, which change no verdict except under `EOLYMP_STRICT` or
  `eo-judge --strict`: EO206 (points the judge's float rounds up to the cost), EO208 (now a
  warning, and right that the run is accepted), EO213, EO409 and EO112.
- eo-judge reads `problem.json` strictly: unknown or wrongly cased fields, trailing content,
  values outside the platform's enums, repeated keys and repeated names are refused, and
  `DONT_RUN` solutions are skipped. Solutions run outside the workspace, and a test or answer
  rewritten during a run ends it with exit 3. Emulated verdicts and scores for valid problems
  are unchanged.

### Also

- Performance: numbers and tokens are read a run of bytes at a time, element names and
  bounds are looked up only when needed, the warning report is ordered without a stable sort,
  and shapes and distinct draws use flat hash sets; every change produces identical output.
- eo-judge: flags anywhere on the command line, `version`, `-v` per-run lines, parallel builds
  that build each recipe once, attached headers found after the system's, whole process groups
  stopped on a timeout or an interrupt, and released binaries for Linux and macOS under
  `judge/v<version>`, with the same version as the header.
- CI: the suite at `-O2` in three standards on GCC, clang, musl and macOS; ASan and UBSan;
  mutants; libFuzzer harnesses on every change and nightly; a version check on every pull
  request; and releases made only from a commit every job passed.
- [docs/testlib.md](docs/testlib.md) maps testlib calls to eolymp.h.
