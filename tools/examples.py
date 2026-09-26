#!/usr/bin/env python3
import os
import pathlib
import re
import shutil
import subprocess
import sys

BLOCK = re.compile(r"```cpp\n(.*?)```", re.S)


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    build = root / "build" / "examples"
    shutil.rmtree(build, ignore_errors=True)
    build.mkdir(parents=True, exist_ok=True)
    compiler = os.environ.get("CXX", "c++")
    standard = os.environ.get("CXXSTD", "c++17")
    compiled = 0
    broken = 0
    pages = sorted((root / "docs").glob("*.md")) + [root / "README.md"]
    for page in pages:
        for number, code in enumerate(BLOCK.findall(page.read_text()), start=1):
            if "int main" not in code:
                continue
            source = build / f"{page.stem}_{number}.cpp"
            source.write_text(code)
            finished = subprocess.run(
                [compiler, f"-std={standard}", "-O1", "-Wall", "-Wextra", "-Wshadow", "-Werror",
                 f"-I{root}", "-c", "-o", str(source.with_suffix(".o")), str(source)],
                capture_output=True, text=True, cwd=root)
            compiled += 1
            if finished.returncode != 0:
                broken += 1
                print(f"  {page.name} example {number} does not build:")
                for line in finished.stderr.strip().splitlines()[:6]:
                    print(f"    {line}")
    print(f"examples: {compiled} from the pages compiled, {broken} broken")
    return 1 if broken else 0


if __name__ == "__main__":
    raise SystemExit(main())
