#!/usr/bin/env python3
import re
import subprocess
import sys

from common import ROOT

RELEASED = ["eolymp.h", "eolymp-shapes.h", "judge", ":(exclude)judge/*_test.go", ":(exclude)judge/testdata"]
HEADER_VERSION = re.compile(r'^#define EOLYMP_H_VERSION "(.*)"$', re.M)
JUDGE_VERSION = re.compile(r'^const version = "(.*)"$', re.M)
SEMVER = re.compile(r"^(\d+)\.(\d+)\.(\d+)(?:-([0-9A-Za-z.-]+))?(?:\+[0-9A-Za-z.-]+)?$")


class Refused(Exception):
    pass


def version_in(text, where):
    found = HEADER_VERSION.search(text)
    if found is None:
        raise Refused(f"{where} defines no EOLYMP_H_VERSION")
    return found.group(1)


def version(ref):
    shown = subprocess.run(["git", "show", f"{ref}:src/core.h"], capture_output=True, text=True)
    if shown.returncode != 0:
        raise Refused(f"cannot read src/core.h at {ref}; fetch that commit, or pass BASE=<ref>")
    return version_in(shown.stdout, f"src/core.h at {ref}")


def current():
    return version_in((ROOT / "src" / "core.h").read_text(), "src/core.h")


def judge_disagrees(main_go, header):
    stated = JUDGE_VERSION.search(main_go)
    if stated is not None and stated.group(1) == header:
        return None
    return (f"judge/main.go says eo-judge is {stated.group(1) if stated else 'unversioned'}, "
            f"but EOLYMP_H_VERSION is {header}; the two are one version")


def print_current():
    try:
        header = current()
    except Refused as reason:
        print(f"version: {reason}", file=sys.stderr)
        return 1
    disagreement = judge_disagrees((ROOT / "judge" / "main.go").read_text(), header)
    if disagreement:
        print(f"version: {disagreement}", file=sys.stderr)
        return 1
    print(header)
    return 0


def ordered(text):
    found = SEMVER.match(text)
    if found is None:
        raise Refused(f"EOLYMP_H_VERSION \"{text}\" is not a semantic version such as 1.2.3 or 1.2.3-rc1")
    release = tuple(int(found.group(part)) for part in (1, 2, 3))
    if found.group(4) is None:
        return release + (1, ())
    identifiers = tuple((0, int(one), "") if one.isdigit() else (1, 0, one)
                        for one in found.group(4).split("."))
    return release + (0, identifiers)


def main() -> int:
    if sys.argv[1:] == ["--print"]:
        return print_current()
    base = sys.argv[1] if len(sys.argv) > 1 else "origin/main"
    try:
        before, after = version(base), version("HEAD")
        earlier, later = ordered(before), ordered(after)
    except Refused as reason:
        print(f"version: {reason}", file=sys.stderr)
        return 1
    judge = subprocess.run(["git", "show", "HEAD:judge/main.go"], capture_output=True, text=True)
    disagreement = judge_disagrees(judge.stdout, after)
    if disagreement:
        print(f"version: {disagreement}", file=sys.stderr)
        return 1
    changed = subprocess.run(["git", "diff", "--quiet", base, "HEAD", "--", *RELEASED]).returncode != 0
    if later < earlier:
        print(f"version: EOLYMP_H_VERSION went back from {before} to {after}; raise it instead",
              file=sys.stderr)
        return 1
    if changed and later == earlier:
        still = before if before == after else f"{after}, which ranks the same as {before}"
        print(f"version: the headers or eo-judge changed but EOLYMP_H_VERSION is still {still}; "
              f"raise it in src/core.h and judge/main.go, or the change never reaches a release",
              file=sys.stderr)
        return 1
    print(f"version: {before} -> {after}" if before != after else f"version: {after}, nothing released changed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
