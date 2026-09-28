#!/usr/bin/env python3
"""Every warning code the library or eo-judge can raise must be in docs/warnings.md.

The page is what an author, or an agent reading a report, looks a code up in, so a code
that exists in the sources and not on the page is a defect the same way an untested line
is. This compares what the sources raise with what the page documents.
"""
import pathlib
import re
import sys

ROW = re.compile(r"^\|\s*`(EO\d{3})`\s*\|")
PLANNED = re.compile(r"^\|\s*`(EO\d{3})`\s*\|.*\bnot built\b", re.IGNORECASE)


def raised(root: pathlib.Path) -> set:
    codes = set()
    for path in sorted(root.glob("src/**/*.h")):
        text = path.read_text()
        codes |= set(re.findall(r'\b(?:detail::)?(?:warn|note|warn_at_once)\(\s*"(EO\d{3})"', text))
        codes |= set(re.findall(r'\bstart_the_clock\("(EO\d{3})"', text))
        for found in re.finditer(r'\[\[deprecated\("eolymp (EO\d{3})(?: and (EO\d{3}))?', text):
            codes.add(found.group(1))
            if found.group(2):
                codes.add(found.group(2))
    for path in sorted(root.glob("judge/*.go")):
        if path.name.endswith("_test.go"):
            continue
        text = path.read_text()
        codes |= set(re.findall(r'\.(?:warn|note)\("(EO\d{3})"', text))
        codes |= set(re.findall(r'"(EO\d{3})",\n\s*"', text))
    return codes


ATTRIBUTES = {
    "EO305": ("src/structure.h", "class [[nodiscard]] check_result"),
    "EO506": ("src/random.h", "[[nodiscard]] long long uniform"),
}


def attributed(root: pathlib.Path) -> set:
    codes = set()
    for code, (where, marker) in ATTRIBUTES.items():
        path = root / where
        if path.exists() and marker in path.read_text():
            codes.add(code)
    return codes


def documented(page: pathlib.Path) -> tuple:
    every, planned, twice = set(), set(), set()
    for line in page.read_text().splitlines():
        found = ROW.match(line)
        if not found:
            continue
        code = found.group(1)
        if code in every:
            twice.add(code)
        every.add(code)
        if PLANNED.match(line):
            planned.add(code)
    return every, planned, twice


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    page = root / "docs" / "warnings.md"
    if not page.exists():
        print("warnings: docs/warnings.md is missing", file=sys.stderr)
        return 1

    built = raised(root) | attributed(root)
    every, planned, twice = documented(page)

    complaints = []
    for code in sorted(built - every):
        complaints.append(f"{code} is raised in the sources and not on the page")
    for code in sorted(every - built - planned):
        complaints.append(f"{code} is on the page as built, but nothing raises it")
    for code in sorted(planned & built):
        complaints.append(f"{code} is on the page as not built, but the sources raise it")
    for code in sorted(twice):
        complaints.append(f"{code} has more than one row")

    print(f"warnings: {len(built)} codes raised, {len(every)} documented, "
          f"{len(planned)} of them not built yet")
    for one in complaints:
        print(f"  {one}")
    return 1 if complaints else 0


if __name__ == "__main__":
    raise SystemExit(main())
