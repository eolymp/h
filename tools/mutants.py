#!/usr/bin/env python3
"""Every line of both headers runs, which is a weak thing to know.

This changes one operator or bound at a time and requires the suite to notice. A mutant
that survives is a line the tests execute without checking what it is for. The table is
the set an outside review found surviving, plus what has been added since.
"""
import concurrent.futures
import os
import pathlib
import shlex
import shutil
import subprocess
import sys
import tempfile

MUTANTS = [
    ("a sum limit that allows one more", "src/validate.h",
     "if (total_ > limit_)", "if (total_ > limit_ + 1)"),
    ("negative points let through down to -1", "src/role.h",
     "    if (paid < 0) {", "    if (paid < -1) {"),
    ("a token one character too long", "src/stream.h",
     "if (length > most)\n                refuse(name,",
     "if (length > most + 1)\n                refuse(name,"),
    ("a line one character too short", "src/stream.h",
     "if (bounds == stated::yes && length < least)", "if (bounds == stated::yes && length < least - 1)"),
    ("a real with one digit too many", "src/stream.h",
     "parsed.decimals > most_decimals", "parsed.decimals > most_decimals + 1"),
    ("a self loop in a simple graph", "src/structure.h",
     "if (one.u == one.v) return check_result", "if (false) return check_result"),
    ("a tolerance that excludes its bound", "src/role.h",
     "if (spread <= epsilon) return true;", "if (spread < epsilon) return true;"),
    ("a score that floors instead of rounding", "src/role.h",
     "return std::round(value * scale) / scale;", "return std::floor(value * scale) / scale;"),
    ("an answer whose trailing spaces count", "src/check.h",
     "while (!want.empty() && (want.back() == ' ' || want.back() == '\\t')) want.pop_back();", ""),
    ("every random stream the same", "src/generate.h",
     "return dice_.emplace(label, eo::rng(from)).first->second;",
     "return dice_.emplace(label, eo::rng(base_)).first->second;"),
    ("a carriage return counted in a line's length", "src/stream.h",
     "            text.pop_back();\n            seen--;", "            text.pop_back();"),
    ("a token read past the cap it was given", "src/stream.h",
     "            if (cap > 0 && static_cast<long long>(token.size()) >= cap) break;", ""),
    ("a look ahead that reads", "src/io.h",
     "std::string ahead(std::size_t limit) const {\n        return std::string(",
     "std::string ahead(std::size_t limit) {\n        have(limit);\n        return std::string("),
]


def run_a_copy(root, compiler, where=None, old=None, new=None):
    with tempfile.TemporaryDirectory(prefix="eolymp-mutant-") as scratch:
        copy = pathlib.Path(scratch)
        for part in ("src", "tools", "tests"):
            shutil.copytree(root / part, copy / part)
        if where is not None:
            (copy / where).write_text((root / where).read_text().replace(old, new))
        subprocess.run([sys.executable, "tools/amalgamate.py"], cwd=copy, check=True,
                       capture_output=True)
        built = subprocess.run([*compiler, "-std=c++17", "-O1", "-DEOLYMP_TESTING", "-o", "mutant",
                                "tests/all.cpp"], cwd=copy, capture_output=True)
        if built.returncode != 0:
            return "does not compile"
        try:
            ran = subprocess.run(["./mutant"], cwd=copy, capture_output=True, timeout=30)
        except subprocess.TimeoutExpired:
            return "timed out"
        return "passed" if ran.returncode == 0 else "failed"


def attempt(root, mutant, compiler):
    name, where, old, new = mutant
    before = (root / where).read_text()
    if before.count(old) != 1:
        return name, f"its anchor appears {before.count(old)} times in {where}"
    outcome = run_a_copy(root, compiler, where, old, new)
    if outcome == "does not compile":
        return name, "the mutant does not compile"
    return name, "survived" if outcome == "passed" else None


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    compiler = shlex.split(os.environ.get("CXX", "c++"))
    workers = int(os.environ.get("JOBS", os.cpu_count() or 1))
    with concurrent.futures.ThreadPoolExecutor(workers) as pool:
        control = pool.submit(run_a_copy, root, compiler)
        outcomes = list(pool.map(lambda one: attempt(root, one, compiler), MUTANTS))
    if control.result() != "passed":
        print(f"mutants: the unchanged sources, copied and built the same way, {control.result()}; "
              f"no mutant can be said to be killed")
        return 1
    failed = [(name, why) for name, why in outcomes if why]
    print(f"mutants: {len(MUTANTS) - len(failed)} of {len(MUTANTS)} killed")
    for name, why in failed:
        print(f"  {name}: {why}")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
