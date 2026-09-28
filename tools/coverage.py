#!/usr/bin/env python3
import os
import pathlib
import re
import shutil
import subprocess
import sys

MEASURED = re.compile(r"^\s*(#####|=====|\d+\*?):\s*(\d+):(.*)$")

HEADERS = ["eolymp.h", "eolymp-shapes.h"]


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    build = root / "build" / "cov"
    shutil.rmtree(build, ignore_errors=True)
    build.mkdir(parents=True, exist_ok=True)
    compiler = os.environ.get("CXX", "c++")
    standard = os.environ.get("CXXSTD", "c++17")
    gcov = os.environ.get("GCOV", "gcov").split()
    if shutil.which(gcov[0]) is None:
        print(f"coverage: {gcov[0]} is not on the path; set GCOV to the coverage tool of {compiler}",
              file=sys.stderr)
        return 1
    described = subprocess.run(gcov + ["--version"], capture_output=True, text=True)
    authoritative = "LLVM" not in described.stdout
    every_inline = ["-fkeep-inline-functions"] if authoritative else []
    subprocess.run(
        [compiler, f"-std={standard}", "-O0", "-g", "--coverage", *every_inline, "-DEOLYMP_TESTING",
         "-o", str(build / "tests"), "tests/all.cpp"],
        cwd=root, check=True)
    subprocess.run([str(build / "tests")], cwd=build, check=True)
    notes = sorted(build.glob("*.gcno"))
    if not notes:
        print("coverage: the compiler produced no .gcno files", file=sys.stderr)
        return 1
    subprocess.run(gcov + ["-r", "-o", str(build)] + [str(note) for note in notes],
                   cwd=root, check=True, stdout=subprocess.DEVNULL)
    unrun = 0
    for header in HEADERS:
        report = root / f"{header}.gcov"
        if not report.exists():
            print(f"coverage: gcov produced no report for {header}", file=sys.stderr)
            return 1
        ran = {}
        source = {}
        for line in report.read_text(errors="replace").splitlines():
            found = MEASURED.match(line)
            if not found:
                continue
            number = int(found.group(2))
            source[number] = found.group(3).strip()
            ran[number] = ran.get(number, False) or found.group(1) not in ("#####", "=====")
        missed = []
        braces = 0
        total = len(ran)
        for number in sorted(ran):
            if ran[number]:
                continue
            text = source[number]
            if "LCOV_EXCL" in text:
                total -= 1
                continue
            if text in ("}", "};"):
                braces += 1
                total -= 1
                continue
            missed.append((number, text))
        covered = total - len(missed)
        counted = f"{header}: {covered}/{total} lines covered"
        if braces:
            counted += f", {braces} closing braces this tool attributes to no statement ignored"
        print(counted)
        for number, text in missed:
            print(f"  {header}:{number}: never run: {text}")
        unrun += len(missed)
    for stale in root.glob("*.gcov"):
        stale.unlink()
    if not unrun:
        return 0
    if not authoritative:
        print(f"coverage: {unrun} lines look unrun, but this is an LLVM gcov, which loses a block "
              f"whose only exit is a throw; the gate runs under GNU gcov")
        return 0
    print(f"coverage: {unrun} lines are never run", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
