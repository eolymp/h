#!/usr/bin/env python3
import concurrent.futures
import os
import shutil
import subprocess

from common import ROOT, compiler, cpp_blocks, standard, warnings


def main() -> int:
    root = ROOT
    build = root / "build" / "examples"
    shutil.rmtree(build, ignore_errors=True)
    build.mkdir(parents=True, exist_ok=True)
    command = [*compiler(), f"-std={standard()}", "-O1", *warnings(), f"-I{root}", "-c"]
    examples = []
    pages = sorted((root / "docs").glob("*.md")) + [root / "README.md"]
    for page in pages:
        for number, code in enumerate(cpp_blocks(page), start=1):
            if "int main" not in code:
                continue
            stem = page.relative_to(root).with_suffix("").as_posix().replace("/", "_")
            source = build / f"{stem}_{number}.cpp"
            source.write_text(code)
            examples.append((page, number, source))

    def build_one(example):
        source = example[2]
        return example, subprocess.run(
            [*command, "-o", str(source.with_suffix(".o")), str(source)],
            capture_output=True, text=True, cwd=root)

    compiled = len(examples)
    broken = 0
    with concurrent.futures.ThreadPoolExecutor(os.cpu_count()) as pool:
        for (page, number, _), finished in pool.map(build_one, examples):
            if finished.returncode != 0:
                broken += 1
                print(f"  {page.relative_to(root)} example {number} does not build:")
                for line in finished.stderr.strip().splitlines()[:6]:
                    print(f"    {line}")
    print(f"examples: {compiled} from the pages compiled, {broken} broken")
    return 1 if broken else 0


if __name__ == "__main__":
    raise SystemExit(main())
