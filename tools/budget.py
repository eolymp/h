#!/usr/bin/env python3
"""What the header costs the programs the pages show: compile time and object size.

The validator is the first example in docs/README.md and the checker the first in
docs/checker.md, so the measurement follows what an author actually writes. Each is compared
with a file that includes only the standard headers eolymp.h uses.
"""
import os
import pathlib
import re
import resource
import subprocess
import sys

CEILING_RATIO = 9.0
BLOCK = re.compile(r"```cpp\n(.*?)```", re.S)

BASELINE = """\
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
int main() { return 0; }
"""


def first_program(page: pathlib.Path) -> str:
    for code in BLOCK.findall(page.read_text()):
        if "int main" in code:
            return code
    raise SystemExit(f"budget: {page} has no program to measure")


def cpu_of_children():
    usage = resource.getrusage(resource.RUSAGE_CHILDREN)
    return usage.ru_utime + usage.ru_stime


def fastest(command, root, runs=3):
    best = float("inf")
    for _ in range(runs):
        before = cpu_of_children()
        subprocess.run(command, cwd=root, check=True)
        best = min(best, cpu_of_children() - before)
    return best


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    build = root / "build" / "budget"
    build.mkdir(parents=True, exist_ok=True)
    programs = {
        "baseline": BASELINE,
        "validator": first_program(root / "docs" / "README.md"),
        "checker": first_program(root / "docs" / "checker.md"),
    }
    compiler = os.environ.get("CXX", "c++")
    standard = os.environ.get("CXXSTD", "c++17")
    measured = {}
    for name, code in programs.items():
        source = build / f"{name}.cpp"
        source.write_text(code)
        common = [compiler, f"-std={standard}", f"-I{root}"]
        parse = fastest(common + ["-fsyntax-only", str(source)], root)
        target = build / f"{name}.o"
        compile_time = fastest(common + ["-O2", "-c", "-o", str(target), str(source)], root)
        measured[name] = (parse, compile_time, target.stat().st_size)
    base = measured["baseline"]
    worst = 0.0
    for name in ("validator", "checker"):
        parse, compile_time, size = measured[name]
        worst = max(worst, compile_time / base[1])
        print(f"budget: the {name} takes {parse:.2f}s of compiler CPU time to parse and {compile_time:.2f}s to "
              f"build with -O2, into {size // 1024} KB, {compile_time / base[1]:.1f} times the standard headers "
              f"alone, which take {base[0]:.2f}s and {base[1]:.2f}s")
    if worst > CEILING_RATIO:
        print(f"budget: an -O2 build takes {worst:.1f} times the compiler CPU time of the standard headers "
              f"alone, above the ceiling of {CEILING_RATIO}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
