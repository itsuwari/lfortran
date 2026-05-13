#!/usr/bin/env python3
"""Report generated C source budget for LFortran C-emitted builds."""

from __future__ import annotations

import argparse
import json
import os
import tempfile
from collections import OrderedDict
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


PATTERNS = OrderedDict(
    [
        ("struct_deepcopy", "struct_deepcopy_"),
        ("struct_cleanup", "cleanup_struct_"),
        ("struct_runtime_registration", "_lfortran_register_c_struct"),
        ("tbp_parent_registration", "_lfortran_register_c_type_parent"),
        ("compiler_created_values", "__libasr_created__"),
        ("array_views", "__lfortran_array_view"),
        ("array_constants", "array_constant_"),
        ("typed_const_data_units", "__lfortran_const_data_"),
        ("compact_const_blobs", "__lfortran_const_blob"),
        ("compact_const_copy_helpers", "__lfortran_const_copy"),
        ("malloc_calls", "_lfortran_malloc"),
        ("realloc_calls", "_lfortran_realloc"),
        ("sum_helpers", "lcompilers_Sum"),
        ("product_helpers", "lcompilers_Product"),
        ("memcpy_calls", "memcpy("),
        ("memset_calls", "memset("),
    ]
)


@dataclass
class FileBudget:
    path: Path
    bytes: int
    lines: int


@dataclass
class NinjaEdge:
    log: Path
    target: str
    millis: int


def line_count(data: bytes) -> int:
    if not data:
        return 0
    count = data.count(b"\n")
    if not data.endswith(b"\n"):
        count += 1
    return count


def read_file_budget(path: Path) -> FileBudget:
    data = path.read_bytes()
    return FileBudget(path=path, bytes=len(data), lines=line_count(data))


def find_files(roots: Iterable[Path], suffix: str) -> list[Path]:
    files: list[Path] = []
    for root in roots:
        if root.is_file() and root.suffix == suffix:
            files.append(root)
        elif root.is_dir():
            files.extend(path for path in root.rglob(f"*{suffix}") if path.is_file())
    return sorted(set(files))


def find_split_dirs(roots: Iterable[Path]) -> list[Path]:
    dirs: list[Path] = []
    for root in roots:
        if root.is_dir():
            dirs.extend(path for path in root.rglob("*.tmp.split") if path.is_dir())
    return sorted(set(dirs))


def display_path(path: Path, roots: list[Path]) -> str:
    candidates = [path]
    for root in roots:
        try:
            candidates.append(path.relative_to(root))
        except ValueError:
            pass
    return str(min(candidates, key=lambda item: len(str(item))))


def count_patterns(paths: Iterable[Path]) -> OrderedDict[str, int]:
    counts: OrderedDict[str, int] = OrderedDict((name, 0) for name in PATTERNS)
    for path in paths:
        text = path.read_text(errors="replace")
        for name, pattern in PATTERNS.items():
            counts[name] += text.count(pattern)
    return counts


def parse_ninja_logs(roots: Iterable[Path]) -> list[NinjaEdge]:
    logs: list[Path] = []
    for root in roots:
        if root.is_file() and root.name == ".ninja_log":
            logs.append(root)
        elif root.is_dir():
            logs.extend(path for path in root.rglob(".ninja_log") if path.is_file())

    edges: list[NinjaEdge] = []
    for log in sorted(set(logs)):
        for line in log.read_text(errors="replace").splitlines():
            if not line or line.startswith("#"):
                continue
            parts = line.split("\t")
            if len(parts) < 4:
                continue
            try:
                start = int(parts[0])
                end = int(parts[1])
            except ValueError:
                continue
            target = parts[3]
            if ".c.o" not in target and not target.endswith(".o"):
                continue
            edges.append(NinjaEdge(log=log, target=target, millis=max(0, end - start)))
    return sorted(edges, key=lambda edge: edge.millis, reverse=True)


def mib(value: int) -> float:
    return value / (1024.0 * 1024.0)


def summarize(roots: list[Path], top: int) -> dict[str, object]:
    c_paths = find_files(roots, ".c")
    h_paths = find_files(roots, ".h")
    c_budgets = [read_file_budget(path) for path in c_paths]
    h_budgets = [read_file_budget(path) for path in h_paths]
    c_budgets.sort(key=lambda item: item.bytes, reverse=True)
    h_budgets.sort(key=lambda item: item.bytes, reverse=True)

    pattern_counts = count_patterns(c_paths)
    ninja_edges = parse_ninja_logs(roots)
    split_dirs = find_split_dirs(roots)

    def file_entry(item: FileBudget) -> dict[str, object]:
        return {
            "path": display_path(item.path, roots),
            "bytes": item.bytes,
            "lines": item.lines,
        }

    return {
        "roots": [str(root) for root in roots],
        "split_dirs": len(split_dirs),
        "c": {
            "files": len(c_budgets),
            "bytes": sum(item.bytes for item in c_budgets),
            "lines": sum(item.lines for item in c_budgets),
            "largest": [file_entry(item) for item in c_budgets[:top]],
        },
        "headers": {
            "files": len(h_budgets),
            "bytes": sum(item.bytes for item in h_budgets),
            "lines": sum(item.lines for item in h_budgets),
            "largest": [file_entry(item) for item in h_budgets[:top]],
        },
        "patterns": pattern_counts,
        "compile_time": [
            {
                "log": display_path(edge.log, roots),
                "target": edge.target,
                "millis": edge.millis,
            }
            for edge in ninja_edges[:top]
        ],
        "compile_rss": {
            "available": False,
            "note": "per-file RSS requires an external compiler wrapper log",
        },
    }


def print_file_table(title: str, entries: list[dict[str, object]]) -> None:
    print(title)
    if not entries:
        print("  none")
        return
    for entry in entries:
        print(
            f"  {entry['bytes']:>10} bytes  {entry['lines']:>8} lines  "
            f"{entry['path']}"
        )


def print_report(report: dict[str, object]) -> None:
    c_info = report["c"]
    h_info = report["headers"]
    assert isinstance(c_info, dict)
    assert isinstance(h_info, dict)

    print("Generated C budget")
    print("Roots:")
    for root in report["roots"]:
        print(f"  {root}")
    print(f"Split directories: {report['split_dirs']}")
    print(
        f"C sources: {c_info['files']} files, {c_info['bytes']} bytes "
        f"({mib(int(c_info['bytes'])):.2f} MiB), {c_info['lines']} lines"
    )
    print(
        f"Headers: {h_info['files']} files, {h_info['bytes']} bytes "
        f"({mib(int(h_info['bytes'])):.2f} MiB), {h_info['lines']} lines"
    )
    print()
    print_file_table("Largest C translation units:", c_info["largest"])
    print()
    print_file_table("Largest headers:", h_info["largest"])
    print()
    print("Helper pattern counts in .c files:")
    patterns = report["patterns"]
    assert isinstance(patterns, dict)
    for name, count in patterns.items():
        print(f"  {name:32s} {count}")
    print()
    print("Longest Ninja compile edges, if .ninja_log is present:")
    compile_time = report["compile_time"]
    assert isinstance(compile_time, list)
    if not compile_time:
        print("  none")
    for entry in compile_time:
        print(f"  {entry['millis']:>8} ms  {entry['target']}")
    compile_rss = report["compile_rss"]
    assert isinstance(compile_rss, dict)
    if not compile_rss.get("available"):
        print(f"Compile RSS: {compile_rss['note']}")


def run_self_test() -> None:
    with tempfile.TemporaryDirectory(prefix="lfortran-c-budget-") as tmp:
        root = Path(tmp)
        split = root / "case.o.tmp.split"
        split.mkdir()
        (split / "case_generated.h").write_text("void f(void);\n")
        (split / "case_pack_0.c").write_text(
            "int __libasr_created__x;\n"
            "void struct_deepcopy_box(void) {}\n"
            "void g(void) { memcpy(0, 0, 0); }\n"
        )
        (split / "case_constants_data.c").write_text(
            "static const unsigned char __lfortran_const_blob0[2] = {1, 2};\n"
            "void __lfortran_const_copy0(void *dst, int64_t stride) {}\n"
        )
        (root / ".ninja_log").write_text(
            "# ninja log v5\n"
            "0\t12\t0\tcase.o.tmp.split/case_pack_0.c.o\tabc\n"
        )
        report = summarize([root], top=5)
        assert report["split_dirs"] == 1
        assert report["c"]["files"] == 2
        assert report["headers"]["files"] == 1
        assert report["patterns"]["compiler_created_values"] == 1
        assert report["patterns"]["struct_deepcopy"] == 1
        assert report["patterns"]["compact_const_blobs"] == 1
        assert report["patterns"]["compact_const_copy_helpers"] == 1
        assert report["patterns"]["memcpy_calls"] == 1
        assert report["compile_time"][0]["millis"] == 12
    print("self-test passed")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Report generated C size and helper-pattern budget."
    )
    parser.add_argument(
        "roots",
        nargs="*",
        type=Path,
        help="build directory, split directory, or generated C file to scan",
    )
    parser.add_argument("--top", type=int, default=20, help="rows to show")
    parser.add_argument("--json", type=Path, help="write machine-readable report")
    parser.add_argument("--self-test", action="store_true", help="run script self-test")
    args = parser.parse_args()

    if args.self_test:
        run_self_test()
        return 0

    roots = [path.resolve() for path in args.roots]
    if not roots:
        parser.error("at least one root is required unless --self-test is used")
    missing = [str(path) for path in roots if not path.exists()]
    if missing:
        parser.error("missing path(s): " + ", ".join(missing))
    report = summarize(roots, top=max(0, args.top))
    print_report(report)
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, indent=2, sort_keys=False) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
