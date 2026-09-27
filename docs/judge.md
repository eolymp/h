# eo-judge

`eo-judge` runs a problem the way the Eolymp judge does, on your machine, and then runs the
checks that no single program can make from inside one run. It lives in [judge/](../judge),
is written in Go with no dependencies beyond the standard library, and is built with
`make judge` or `cd judge && go build .`.

```bash
eo-judge run   <problem>   # build, generate, validate, judge every solution, score it
eo-judge check <problem>   # EO801-EO821 and EO901-EO910
eo-judge lint  <problem>   # what is only visible in the source
eo-judge version           # the version of eo-judge
```

| Flag | Does |
| --- | --- |
| `--solution name` | judge one solution instead of all of them; a name the problem does not have is a usage error that lists the names it has |
| `--strict` | exit non-zero if anything raised a warning |
| `--deep` | use the full 100 MB hostile output rather than 2 MB |
| `--work dir` | keep the workspace instead of a temporary directory |

Flags may come before or after the problem directory, and `-h` or `--help` prints the usage.
It exits 0 when it finished, 1 under `--strict` with warnings, 2 on a usage error and 3 when
the problem itself could not be run — a program that does not compile, a generator that
fails, a missing file.

## It reproduces the judge, deliberately

Every scoring rule is taken from the platform's own source rather than from a specification,
because the two have disagreed before:

| Rule | Comes from |
| --- | --- |
| exit 0 accepted, 1 and 2 wrong answer, **7** a fraction, anything else a system failure | agent `internal/judge/checker/program.go` |
| the environment a checker is given: `EOLYMP`, `INPUT_FILE`, `OUTPUT_FILE`, `ANSWER_FILE`, `TEST_ID`, `TEST_COST`, `TEST_INDEX`, `TEST_GROUP` | the same file |
| an interactor's arguments and environment: the input, the output, and the answer only when the test has one; the run's metadata merged in; a limit of the solution's plus a second | agent `internal/judge/runner/script.go`, `interact()` |
| `readPoints`, which scans the log for the word `points` and then a float | copied verbatim from the same file, and a test diffs the copy against `origin/main` |
| an interactor's exit code: 0 runs the checker, 1 and 2 are a wrong answer, anything else an interaction failure — but only once the solution's own run completed, so a crash is never excused | agent `internal/judge/runner/script.go`, `run()`; the rule that forgives a broken pipe lives in `communicate()` and applies to COMMUNICATION only |
| the ICPC stop, which trips on a wrong answer worth **zero** and not on a partial score | agent `internal/judge/admissioner/showstopper.go` |
| `FULLY_ACCEPTED` and `FIRST_POINT` dependencies | agent `internal/judge/admissioner/dependency.go` |
| accepted pays the cost, a wrong answer and a partially correct run pay `min(cost, score)`, anything else pays nothing | atlas `internal/services/submissions/submission_reporter.go` |
| a checker's exit 7 is `PARTIALLY_CORRECT`, and `ACCEPTED` when its points reach the test's cost — which a test worth 0 always does | agent `internal/judge/evaluation.go` |
| `ALL` pays only if every run passed, `EACH` sums, `WORST` takes the smallest, `BEST` the largest | the same file |

The two problems in [../tests/live](../tests/live) are what holds this emulation to the
judge's behaviour: a batch problem with a partial-scoring checker and an interactive one with a
query budget. The `eo-judge` tests run both end to end and assert the verdict and the score of
all seven submissions, and those expected values are what the real judge awarded — so a change
that drifts from the judge fails the suite.

## The problem directory

A problem is a directory with a `problem.json` beside its sources. The field names follow the
Eolymp API, so a problem exported from the platform maps onto it one to one.

```json
{
  "title": "tree degrees",
  "type": "PROGRAM",
  "timeLimit": 2000,
  "memoryLimit": 268435456,
  "uniqueAnswer": true,
  "checker":   {"source": "checker.cpp",   "runtime": "cpp:20-gnu14", "files": ["../../eolymp.h"]},
  "validator": {"source": "validator.cpp", "runtime": "cpp:20-gnu14", "files": ["../../eolymp.h"]},
  "scripts": {
    "gen":      {"source": "generator.cpp", "files": ["../../eolymp.h", "../../eolymp-shapes.h"]},
    "solution": {"source": "solution_full.cpp"}
  },
  "solutions": [
    {"name": "full",   "source": "solution_full.cpp", "type": "CORRECT"},
    {"name": "leaves", "source": "solution_leaves.cpp"}
  ],
  "testsets": [
    {"index": 1, "scoringMode": "EACH", "feedbackPolicy": "COMPLETE",
     "tests": [
       {"index": 1, "score": 3,
        "generator": {"script": "gen", "arguments": ["-n=6", "-shape=path"]},
        "answerGenerator": "solution"}
     ]}
  ]
}
```

### Problem

| Field | Default | Means |
| --- | --- | --- |
| `type` | `PROGRAM` | `PROGRAM`, `INTERACTIVE` or `COMMUNICATION` |
| `runCount` | 1 | how many times a solution runs, chaining the interactor's output into the next run |
| `timeLimit`, `cpuLimit` | — | milliseconds; a testset may override the wall limit |
| `memoryLimit` | — | bytes |
| `uniqueAnswer` | false | the answer is the only correct one, which is what turns EO804 on |
| `exactFormat` | false | whitespace is part of the format, which turns EO818 off |
| `checker`, `validator`, `interactor` | — | one program each |
| `scripts` | — | named generators; `answerGenerator` names one of them |
| `solutions` | — | what `run` judges and `check` compares subtasks against |
| `testsets` | — | the groups |

### Program

| Field | Means |
| --- | --- |
| `source` | the file, relative to the problem directory |
| `runtime` | an Eolymp runtime name; only the C++ standard is read from it |
| `files` | headers copied next to the source before compiling, exactly as the judge's `files[]` does; that directory is searched after the system's headers, so `#include <eolymp.h>` finds an installed copy first, as the judge does, and an attached one when there is none |

`eo-judge` compiles on your machine, not in the judge's runtime image, so a problem that uses
`eolymp.h` still names it in `files` here even though it attaches nothing on the judge.

The source is compiled as `source.cpp`, which is what the judge calls it, so a warning's line
matches what a judge log would say.

### Testset and test

| Field | Default | Means |
| --- | --- | --- |
| `index` | — | 0 is the examples testset |
| `scoringMode` | `EACH` | `EACH`, `ALL`, `WORST` or `BEST` |
| `feedbackPolicy` | `COMPLETE` | `ICPC` stops the group after a test worth nothing |
| `dependencyMode` | `FULLY_ACCEPTED` | or `FIRST_POINT` |
| `dependencies` | — | testset indexes that must pass first |
| `timeLimit`, `memoryLimit` | the problem's | per testset |
| `tests[].score` | 0 | what the test is worth |
| `tests[].input`, `tests[].answer` | — | files, CRLF-normalised the way the judge does |
| `tests[].generator` | — | `{"script": name, "arguments": [...]}` |
| `tests[].answerGenerator` | — | a script name; it is given the input on stdin |
| `tests[].example` | false | a sample |

A test with no `answer` and no `answerGenerator` uses its input as the answer, which is what
an interactive problem wants.

## Reading a run

```
full: ACCEPTED, 100
  testset 1  ACCEPTED                   9 of 9        3 ACCEPTED
  testset 2  ACCEPTED                  91 of 91       7 ACCEPTED

leaves: PARTIALLY_CORRECT, 55.767494
  testset 1  PARTIALLY_CORRECT      5.425 of 9        3 PARTIALLY_CORRECT
  testset 2  PARTIALLY_CORRECT      50.34 of 91       7 PARTIALLY_CORRECT
```

That is the `check_solutions` oracle offline: the score a submission would get, per testset,
before anything is uploaded.

## Reading a check

Every finding carries a code, and every code is in [warnings.md](warnings.md), one
self-contained row each. A report looks like this:

```
testset 1: warning EO807: no test reaches n = 2
  a maximal test that is not maximal; generate one that reaches it
note EO821: the problem has 1 correct solution(s)
  a second correct solution is what a stress run compares the reference with
```

`check` also replays the warnings the programs themselves raised while generating and
validating, so one command covers both halves.

## What it does not do

- **It does not enforce memory.** It measures peak usage and reports it; the time limit is
  enforced, memory is not. A memory-limit verdict is the judge's to give.
- **It does not run `COMMUNICATION` problems.** One interactor against one solution works;
  several instances behind the SPAWN handshake do not yet.
- **It does not fetch a problem from the platform.** The directory is written by hand, or by
  a tool that exports one.
- **It does not read the statement**, so it cannot tell that a bound disagrees with the text.
  EO106 is the nearest thing, and it only notices a bound one away from a round number.
