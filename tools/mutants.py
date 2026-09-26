#!/usr/bin/env python3
"""Every line of both headers runs, which is a weak thing to know.

This changes one operator or bound at a time and requires the suite to notice. A mutant
that survives is a line the tests execute without checking what it is for. The table is
the set an outside review found surviving, plus what has been added since.
"""
import pathlib
import subprocess
import sys

MUTANTS = [
    ("a sum limit that allows one more", "src/validate.h",
     "if (total_ > limit_)", "if (total_ > limit_ + 1)"),
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


def run(root, command, seconds=None):
    return subprocess.run(command, cwd=root, shell=True, capture_output=True, text=True,
                          timeout=seconds)


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    compiler = "c++"
    survivors = []
    broken = []

    for name, where, old, new in MUTANTS:
        path = root / where
        before = path.read_text()
        if before.count(old) != 1:
            broken.append(f"{name}: its anchor appears {before.count(old)} times in {where}")
            continue
        path.write_text(before.replace(old, new))
        try:
            run(root, "python3 tools/amalgamate.py")
            built = run(root, f"{compiler} -std=c++17 -O1 -DEOLYMP_TESTING -o build/mutant tests/all.cpp")
            if built.returncode != 0:
                broken.append(f"{name}: the mutant does not compile")
                continue
            try:
                if run(root, "./build/mutant", seconds=120).returncode == 0:
                    survivors.append(name)
            except subprocess.TimeoutExpired:
                pass
        finally:
            path.write_text(before)

    run(root, "python3 tools/amalgamate.py")

    print(f"mutants: {len(MUTANTS) - len(survivors) - len(broken)} of {len(MUTANTS)} killed")
    for one in survivors:
        print(f"  survived: {one}")
    for one in broken:
        print(f"  {one}")
    return 1 if survivors or broken else 0


if __name__ == "__main__":
    raise SystemExit(main())
