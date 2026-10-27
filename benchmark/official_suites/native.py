#!/usr/bin/env python3
"""Run the official Benchpress pytest harness with QLLVM or supported SDKs."""
import argparse
from collections import Counter
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import subprocess
import sys

from sources import ROOT, verify


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--frameworks", nargs="+", choices=["qllvm", "qiskit", "tket", "pyqpanda3"], default=["qllvm"])
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--selection", default="", help="Optional official pytest -k expression; omitted means all workloads")
    p.add_argument("--collect-only", action="store_true")
    p.add_argument("--compiler", type=Path, default=ROOT.parent.parent / "build/bin/qllvm-compile")
    p.add_argument("--max-qubits", type=int, default=0)
    p.add_argument("--timeout", type=float, default=60)
    p.add_argument("--rounds", type=int, default=3)
    p.add_argument("--library-preload", type=Path, help="Optional process-local compatibility library")
    args = p.parse_args()
    record = verify("benchpress")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    env = dict(os.environ, PYTHONDONTWRITEBYTECODE="1", OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
    env["PYTHONPATH"] = str(ROOT / "upstream/benchpress") + os.pathsep + env.get("PYTHONPATH", "")
    if args.library_preload:
        env["LD_PRELOAD"] = str(args.library_preload)
    rows = []
    for framework in args.frameworks:
        folder = output / framework
        folder.mkdir()
        command = [sys.executable, "-B", "-m", "pytest", "-q", "-p", "no:cacheprovider",
                   "--benchmark-min-rounds=" + str(args.rounds), "--benchmark-max-time=0.1",
                   "--benchmark-json=" + str(folder / "timing.json"), "--json-report",
                   "--json-report-file=" + str(folder / "tests.json"), "--basetemp=" + str(folder / "tmp"),
                   "--timeout=" + str(max(args.timeout + 30, 120))]
        if framework == "qllvm":
            command += [str(ROOT / "benchpress_adapter"), "--qllvm-compiler=" + str(args.compiler.resolve()),
                        "--qllvm-timeout=" + str(args.timeout), "--qllvm-max-qubits=" + str(args.max_qubits)]
        else:
            gym = {"qiskit": "qiskit", "tket": "tket", "pyqpanda3": "qpanda"}[framework]
            command += [str(ROOT / "upstream/benchpress/benchpress" / (gym + "_gym"))]
        if args.selection:
            command += ["-k", args.selection]
        if args.collect_only:
            command += ["--collect-only"]
        print("Running official Benchpress:", framework, flush=True)
        with (folder / "process.log").open("w") as log:
            done = subprocess.run(command, cwd=folder, env=env, stdout=log, stderr=subprocess.STDOUT)
        report_path = folder / "tests.json"
        report = json.loads(report_path.read_text()) if report_path.exists() else {}
        counts = report.get("summary", {})
        row = {"framework": framework, "returncode": done.returncode, "summary": counts,
               "library_preload": str(args.library_preload) if args.library_preload else None,
               "command": command, "selection": args.selection, "scope": "Native Benchpress status definitions and metrics"}
        rows.append(row)
        (output / "results.json").write_text(json.dumps({"source_revision": record["revision"],
            "created_at": datetime.now(timezone.utc).isoformat(), "results": rows}, indent=2) + "\n")
        print(framework, done.returncode, counts, flush=True)
    lines = ["# Benchpress 原生测试结果", "", "原始官方测试、参数、pytest-benchmark 指标及门集/拓扑校验。",
             "QLLVM使用官方工作负载；构建/修改SDK任务按官方默认规则标为不支持。",
             "原生PASSED表示完成官方要求，不等于额外的完整算符等价认证。", "",
             "| 框架 | 通过 | 失败 | 不支持/资源跳过 | XFAIL | 退出码 | 选中范围 |", "|---|---:|---:|---:|---:|---:|---|"]
    for r in rows:
        s = r["summary"]
        lines.append(f"| {r['framework']} | {s.get('passed',0)} | {s.get('failed',0)+s.get('error',0)} | {s.get('skipped',0)} | {s.get('xfailed',0)} | {r['returncode']} | {r['selection'] or '全部官方任务'} |")
    lines += ["", "每个框架目录的 timing.json 提供官方耗时分布与 extra_info 指标，tests.json 提供逐项状态，process.log 保留完整日志。",
              "QLLVM设备映射、数值精度、符号参数等失败会如实保留。完整原生套件可能运行数日，源码完整与实际运行覆盖分别报告。"]
    (output / "summary.md").write_text("\n".join(lines) + "\n")
    return 1 if any(r["returncode"] for r in rows) else 0


if __name__ == "__main__":
    sys.exit(main())
