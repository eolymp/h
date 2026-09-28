import os
import pathlib
import re
import shlex

ROOT = pathlib.Path(__file__).resolve().parent.parent
CPP_BLOCK = re.compile(r"```cpp\n(.*?)```", re.S)


def compiler() -> list:
    return shlex.split(os.environ.get("CXX", "c++"))


def standard() -> str:
    return os.environ.get("CXXSTD", "c++17")


def warnings() -> list:
    stated = os.environ.get("WARNINGS")
    if stated is None:
        found = re.search(r"^WARNINGS := (.*)$", (ROOT / "Makefile").read_text(), re.M)
        if found is None:
            raise SystemExit("the Makefile defines no WARNINGS; set WARNINGS to the compiler's warning flags")
        stated = found.group(1)
    return shlex.split(stated)


def cpp_blocks(page: pathlib.Path) -> list:
    return CPP_BLOCK.findall(page.read_text())
