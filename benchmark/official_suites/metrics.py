#!/usr/bin/env python3
"""Evaluate the unchanged official QASMBench metrics in a compatible environment."""
import argparse
from collections import Counter
import contextlib
import importlib.util
import importlib.metadata
import io
import json
import math
import os
from pathlib import Path
import signal
import subprocess
import sys
from datetime import datetime, timezone

from sources import ROOT, sha, verify


def json_value(value):
    if hasattr(value, "item"):
        value = value.item()
    if isinstance(value, dict):
        return {k: json_value(v) for k, v in value.items()}
    if isinstance(value, (list, tuple)):
        return [json_value(v) for v in value]
    if isinstance(value, float) and not math.isfinite(value):
        return None
    return value


def evaluate(path, output):
    source = ROOT / "upstream/qasmbench/metrics/QMetric.py"
    result = {"input": str(path), "input_sha256": sha(path), "metric_source": str(source),
              "metric_source_sha256": sha(source), "python": sys.version,
              "environment": {n: importlib.metadata.version(n) for n in ["qiskit-terra", "numpy", "networkx", "matplotlib"]},
              "scope": "Unmodified upstream QMetric.evaluate_qasm; original gate definitions, formulas and limitations"}
    log = io.StringIO()
    try:
        spec = importlib.util.spec_from_file_location("official_qasmbench_metrics", source)
        module = importlib.util.module_from_spec(spec)
        with contextlib.redirect_stdout(log), contextlib.redirect_stderr(log):
            spec.loader.exec_module(module)
            metric = module.QMetric(path.read_text())
            values = metric.evaluate_qasm()
        result["metrics"] = json_value(values)
        result["status"] = "computed"
        if "not counted towards evaluation" in log.getvalue():
            result["status"] = "partial"
            result["note"] = "Official implementation does not count one or more input gates; see log"
    except Exception as exc:
        result.update(status="unsupported", error=f"{type(exc).__name__}: {exc}")
    output.with_suffix(".log").write_text(log.getvalue())
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2, allow_nan=False) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--results", type=Path, help="Completed run.py results.json")
    parser.add_argument("--python", type=Path, help="Legacy metric environment; separate from benchmark SDKs")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--timeout", type=float, default=30)
    parser.add_argument("--max-bytes", type=int, default=1024*1024)
    parser.add_argument("--input", type=Path, help="Internal single-input worker or a standalone QASM metric query")
    args = parser.parse_args()
    if args.input:
        evaluate(args.input.resolve(), args.output.resolve())
        return 0
    if not args.results or not args.python:
        parser.error("Use --results and --python for batch metrics")
    verify("qasmbench")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    results = json.loads(args.results.read_text())
    root = args.results.resolve().parent
    inputs, rows, cached = {}, [], {}
    for row in results["results"]:
        if row["suite"] != "qasmbench" or not row.get("artifacts"):
            continue
        folder = root / row["artifacts"]
        request = json.loads((folder / "request.json").read_text())
        for kind, path in [("input", Path(request["input"])), ("output", folder / "compiled.qasm")]:
            if path.is_file():
                inputs[(row["id"], row["framework"], row["repetition"], kind)] = path
    for (case, framework, repetition, kind), path in inputs.items():
        digest = sha(path)
        if digest not in cached:
            dest = output / (digest + ".json")
            if args.max_bytes and path.stat().st_size > args.max_bytes:
                item = {"status": "out_of_scope", "error": "Explicit metric input byte limit"}
            else:
                env = dict(os.environ, OPENBLAS_NUM_THREADS="1", OMP_NUM_THREADS="1", PYTHONDONTWRITEBYTECODE="1", MPLCONFIGDIR=str(output / "matplotlib"))
                with (output / (digest + ".process.log")).open("w") as log:
                    process = subprocess.Popen([os.path.abspath(args.python), "-B", str(Path(__file__).resolve()),
                             "--input", str(path), "--output", str(dest)], stdout=log,
                             stderr=subprocess.STDOUT, env=env, start_new_session=True)
                    try:
                        process.wait(timeout=args.timeout)
                        item = json.loads(dest.read_text()) if dest.exists() else {
                            "status": "failed", "error": f"Metric worker exit {process.returncode}"}
                    except subprocess.TimeoutExpired:
                        os.killpg(process.pid, signal.SIGKILL); process.wait()
                        item = {"status": "timeout", "error": "Official metric timeout"}
            cached[digest] = item
        rows.append({"id": case, "framework": framework, "repetition": repetition,
                     "kind": kind, "input_sha256": digest, **cached[digest]})
    data = {"created_at": datetime.now(timezone.utc).isoformat(), "benchmark_results": str(args.results.resolve()),
            "benchmark_results_sha256": sha(args.results), "counts": dict(Counter(x["status"] for x in rows)),
            "distinct_inputs": len(cached), "results": rows}
    (output / "metrics.json").write_text(json.dumps(data, ensure_ascii=False, indent=2, allow_nan=False) + "\n")
    lines = ["# QASMBench 官方指标", "", "直接执行完整快照中的 QMetric.evaluate_qasm；兼容环境与编译器计时环境分开。",
        "指标计算不计入框架性能；相同内容按哈希复用指标。原版脚本不支持的输入单独记录。", "",
        "| 案例 | 框架 | 输入/输出 | 状态 | 宽度 | 深度 | 门密度 | 保持寿命 | 测量密度 | 纠缠方差 |",
        "|---|---|---|---|---:|---:|---:|---:|---:|---:|"]
    for row in rows:
        if row["repetition"] != 1:
            continue
        m = row.get("metrics", {})
        def number(key):
            v = m.get(key)
            return f"{v:.5g}" if isinstance(v, (int, float)) else "—"
        vals = [row["id"], row["framework"], row["kind"], row["status"]]
        vals += [number(k) for k in ["circuit_width", "circuit_depth", "gate_density",
                    "retention_lifespan", "measurement_density", "entanglement_variance"]]
        lines.append("| " + " | ".join(vals) + " |")
    (output / "summary.md").write_text("\n".join(lines) + "\n")
    print(json.dumps({"report": str(output / "summary.md"), "counts": data["counts"]}))
    return 0


if __name__ == "__main__":
    sys.exit(main())
