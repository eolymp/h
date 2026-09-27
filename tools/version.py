#!/usr/bin/env python3
import re
import subprocess
import sys

RELEASED = ["eolymp.h", "eolymp-shapes.h", "judge", ":(exclude)judge/*_test.go", ":(exclude)judge/testdata"]
JUDGE_VERSION = re.compile(r'^const version = "(.*)"$', re.M)
SEMVER = re.compile(r"^(\d+)\.(\d+)\.(\d+)(?:-([0-9A-Za-z.-]+))?(?:\+[0-9A-Za-z.-]+)?$")


class Refused(Exception):
    pass


def version(ref):
    shown = subprocess.run(["git", "show", f"{ref}:src/core.h"], capture_output=True, text=True)
    if shown.returncode != 0:
        raise Refused(f"cannot read src/core.h at {ref}; fetch that commit, or pass BASE=<ref>")
    found = re.search(r'^#define EOLYMP_H_VERSION "(.*)"$', shown.stdout, re.M)
    if found is None:
        raise Refused(f"src/core.h at {ref} defines no EOLYMP_H_VERSION")
    return found.group(1)


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
    base = sys.argv[1] if len(sys.argv) > 1 else "origin/main"
    try:
        before, after = version(base), version("HEAD")
        earlier, later = ordered(before), ordered(after)
    except Refused as reason:
        print(f"version: {reason}", file=sys.stderr)
        return 1
    judge = subprocess.run(["git", "show", "HEAD:judge/main.go"], capture_output=True, text=True)
    stated = JUDGE_VERSION.search(judge.stdout)
    if stated is None or stated.group(1) != after:
        print(f"version: judge/main.go says eo-judge is {stated.group(1) if stated else 'unversioned'}, "
              f"but EOLYMP_H_VERSION is {after}; the two are one version", file=sys.stderr)
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
