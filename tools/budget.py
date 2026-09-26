#!/usr/bin/env python3
import os
import pathlib
import subprocess
import sys
import time

CEILING_SECONDS = 3.0

BASELINE = """\
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
int main() { return 0; }
"""

LIBRARY = """\
#include "eolymp.h"
int main() { return 0; }
"""


def fastest(command, root, runs=3):
    best = float("inf")
    for _ in range(runs):
        started = time.perf_counter()
        subprocess.run(command, cwd=root, check=True)
        best = min(best, time.perf_counter() - started)
    return best


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    build = root / "build" / "budget"
    build.mkdir(parents=True, exist_ok=True)
    (build / "baseline.cpp").write_text(BASELINE)
    (build / "library.cpp").write_text(LIBRARY)
    compiler = os.environ.get("CXX", "c++")
    standard = os.environ.get("CXXSTD", "c++17")
    measurements = {}
    for name in ("baseline", "library"):
        source = str(build / f"{name}.cpp")
        common = [compiler, f"-std={standard}", f"-I{root}"]
        measurements[name, "parse"] = fastest(common + ["-fsyntax-only", source], root)
        measurements[name, "build"] = fastest(common + ["-O2", "-c", "-o", str(build / f"{name}.o"), source], root)
    for stage in ("parse", "build"):
        library = measurements["library", stage]
        baseline = measurements["baseline", stage]
        print(f"budget: {stage} {library:.2f}s, standard headers alone {baseline:.2f}s, "
              f"eolymp.h adds {library - baseline:.2f}s")
    if measurements["library", "build"] > CEILING_SECONDS:
        print(f"budget: -O2 takes {measurements['library', 'build']:.2f}s, above the {CEILING_SECONDS}s ceiling",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
