#!/usr/bin/env python3
"""Combine completed per-framework runs with matching inputs and settings."""
import argparse
import copy
import json
from pathlib import Path

from sources import sha
from run import write_report


def combine(paths, replacements=()):
    required = ["catalog_sha256", "input_mode", "opt", "repetitions", "warmups", "timeout",
                "memory_gb", "verify_qubits", "tolerance", "selected_ids", "compiler_sha256",
                "versions_sha256", "python", "library_preload", "host"]
    first = json.loads(paths[0].read_text())
    metadata = copy.deepcopy(first["metadata"])
    by_framework = {}
    sources = []
    for path in paths:
        data = json.loads(path.read_text())
        current = data["metadata"]
        if not current.get("complete"):
            raise ValueError("Cannot combine an unfinished run: " + str(path))
        for key in required:
            if current.get(key) != metadata.get(key):
                raise ValueError("Run setting differs: " + key)
        expected = len(current["selected_ids"]) * current["repetitions"]
        for tool in current["frameworks"]:
            if tool in by_framework and tool not in replacements:
                raise ValueError("Duplicate framework needs explicit --replace: " + tool)
            rows = [dict(r, source_run=str(path.resolve())) for r in data["results"] if r["framework"] == tool]
            keys = {(r["id"], r["repetition"]) for r in rows}
            if len(rows) != expected or len(keys) != expected:
                raise ValueError("Incomplete/duplicate framework trials: " + tool)
            # Artifact locations remain valid from the new report location.
            for row in rows:
                if row.get("artifacts"):
                    row["artifacts"] = str((path.parent / row["artifacts"]).resolve())
            by_framework[tool] = rows
        sources.append({"results": str(path.resolve()), "sha256": sha(path), "metadata": current})
    metadata["frameworks"] = list(by_framework)
    metadata["source_runs"] = sources
    metadata["replaced_frameworks"] = list(replacements)
    metadata["report_note"] = "Sequential runs combined by explicit framework replacement; source metadata and artifacts retained"
    rows = [row for tool in metadata["frameworks"] for row in by_framework[tool]]
    return metadata, rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, nargs="+", required=True)
    parser.add_argument("--replace", nargs="*", default=[])
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    metadata, rows = combine(args.input, args.replace)
    args.output.mkdir(parents=True, exist_ok=False)
    write_report(args.output, metadata, rows)
    with (args.output / "summary.md").open("a") as handle:
        handle.write("\n## 复测来源\n\n")
        handle.write("框架按明确指定的复测结果替换，输入、规模、重复次数、超时、版本和核验配置一致。原始运行及每次适配器哈希保存在 results.json 的 source_runs。\n\n")
        for source in metadata["source_runs"]:
            handle.write(f"- {source['results']}\n")
        handle.write("\n替换框架：" + ", ".join(args.replace) + "。\n")
    print(args.output / "summary.md")


if __name__ == "__main__":
    main()
