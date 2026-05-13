#!/usr/bin/env python3
"""Check small split-C generation budgets for focused compiler regressions."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess
import sys


FUNCTION_DEF_RE = re.compile(
    r"^\s*(?!(?:if|for|while|switch)\b)"
    r"(?:[A-Za-z_][\w\s\*]*\s+)+([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{"
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tmpdir", type=Path)
    parser.add_argument("--max-helpers", type=int, required=True)
    parser.add_argument("--max-generated-c-bytes", type=int, required=True)
    parser.add_argument("--max-object-bytes", type=int, required=True)
    parser.add_argument("--max-executable-bytes", type=int, required=True)
    parser.add_argument("--max-text-bytes", type=int, required=True)
    return parser.parse_args()


def file_size(path: Path) -> int:
    return path.stat().st_size if path.is_file() else 0


def generated_split_files(tmpdir: Path) -> list[Path]:
    files: list[Path] = []
    for split_dir in tmpdir.glob("*.o.tmp.split"):
        files.extend(path for path in split_dir.rglob("*") if path.is_file())
    return files


def count_shared_function_defs(files: list[Path]) -> int:
    count = 0
    for path in files:
        if not path.name.endswith("_shared.c"):
            continue
        for line in path.read_text(errors="ignore").splitlines():
            if FUNCTION_DEF_RE.match(line):
                count += 1
    return count


def text_size(path: Path) -> int | None:
    if not path.is_file():
        return None
    try:
        result = subprocess.run(
            ["size", "-m", str(path)],
            text=True,
            capture_output=True,
            check=False,
        )
    except OSError:
        return None
    if result.returncode == 0:
        for line in result.stdout.splitlines():
            match = re.match(r"\s*Section\s+__text:\s+(\d+)\b", line)
            if match:
                return int(match.group(1))

    result = subprocess.run(
        ["size", str(path)],
        text=True,
        capture_output=True,
        check=False,
    )
    if result.returncode != 0:
        return None
    for line in result.stdout.splitlines():
        fields = line.split()
        if fields and fields[0].isdigit():
            return int(fields[0])
    return None


def main() -> int:
    args = parse_args()
    tmpdir: Path = args.tmpdir
    files = generated_split_files(tmpdir)

    generated_c_bytes = sum(file_size(path) for path in files
                            if path.suffix in {".c", ".h"})
    object_bytes = sum(file_size(path) for path in files if path.suffix == ".o")
    object_bytes += file_size(tmpdir / "main.o")
    executable_bytes = file_size(tmpdir / "a.out")
    helper_count = count_shared_function_defs(files)
    executable_text_bytes = text_size(tmpdir / "a.out")

    checks: list[tuple[str, int | None, int]] = [
        ("helper_count", helper_count, args.max_helpers),
        ("generated_c_bytes", generated_c_bytes, args.max_generated_c_bytes),
        ("object_bytes", object_bytes, args.max_object_bytes),
        ("executable_bytes", executable_bytes, args.max_executable_bytes),
    ]
    if executable_text_bytes is not None:
        checks.append(("text_bytes", executable_text_bytes, args.max_text_bytes))

    failures = [
        f"{name}={value} exceeds budget {budget}"
        for name, value, budget in checks
        if value is not None and value > budget
    ]
    if failures:
        print("\n".join(failures), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
