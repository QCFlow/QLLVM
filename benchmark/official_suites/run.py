#!/usr/bin/env python3
"""Automate every official suite and compare QLLVM with current quantum SDKs."""
import argparse
import csv
from collections import Counter, defaultdict
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import signal
import statistics
import subprocess
import sys
import tarfile
import shutil
import time

from sources import ROOT, verify, sha

TOOLS = ["qllvm", "qiskit", "tket", "pyqpanda", "pyqpanda3", "pennylane"]


def bounded(command, directory, timeout, memory_gb=8, preload=None):
    import resource
    def limits():
        if memory_gb:
            resource.setrlimit(resource.RLIMIT_AS, (int(memory_gb * 1024**3),) * 2)
    env = dict(os.environ, OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1", MKL_NUM_THREADS="1",
               NUMEXPR_NUM_THREADS="1", PYTHONDONTWRITEBYTECODE="1")
    if preload:
        env["LD_PRELOAD"] = str(preload)
    with (directory / "process.log").open("w") as log:
        start = time.perf_counter()
        process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT,
                                   cwd=directory, env=env, start_new_session=True, preexec_fn=limits)
        try:
            process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait()
            return {"status": "timeout", "error": f"Worker exceeded {timeout}s", "wall_seconds": time.perf_counter() - start}
    if process.returncode:
        return {"status": "failed", "error": f"Worker process exit {process.returncode}; see process.log"}
    result = directory / "worker.json"
    if not result.is_file():
        return {"status": "failed", "error": "Worker returned without result"}
    return json.loads(result.read_text())


def task(args, request, directory):
    requested = directory
    attempt = 0
    while directory.exists():
        attempt += 1
        directory = requested.with_name(requested.name + "-attempt-" + str(attempt))
    directory.mkdir(parents=True, exist_ok=False)
    request["directory"] = str(directory)
    request_path = directory / "request.json"
    request_path.write_text(json.dumps(request, indent=2) + "\n")
    result = bounded([str(args.python), "-B", str(ROOT / "worker.py"), "--request", str(request_path)],
                     directory, args.timeout, args.memory_gb, args.library_preload)
    result["artifacts"] = str(directory.relative_to(args.output))
    return result


def safe_path(relative):
    p = (ROOT / relative).resolve()
    if ROOT not in p.parents:
        raise ValueError("Catalog path escapes suite root")
    return p


def choose_cases(catalog, args):
    selected = [c for c in catalog["cases"] if not args.suite or c["suite"] in args.suite]
    if args.case:
        selected = [c for c in selected if c["id"] in args.case]
        missing = set(args.case) - {c["id"] for c in selected}
        if missing:
            raise ValueError("Unknown case IDs: " + repr(sorted(missing)))
    if args.profile == "smoke" and not args.case:
        smoke = []
        for suite in sorted({c["suite"] for c in selected}):
            pool = sorted([c for c in selected if c["suite"] == suite and c.get("generation_status") == "ready"
                           and c.get("qubits") and c["qubits"] <= 6],
                          key=lambda c: (c["qubits"], c.get("level") != "alg", c["id"]))
            seen_families, seen_hashes = set(), set()
            for case in pool:
                if case["family"] in seen_families or case["sha256"] in seen_hashes:
                    continue
                smoke.append(case); seen_families.add(case["family"]); seen_hashes.add(case["sha256"])
                if len(seen_families) >= args.smoke_per_suite:
                    break
        selected = smoke
    return selected


def paired_samples(rows, tools, repetitions=1):
    """Compare only identical cases verified by every requested compiler."""
    by_case = defaultdict(lambda: defaultdict(list))
    for row in rows:
        by_case[row["id"]][row["framework"]].append(row)
    return {case: {t: results[t] for t in tools} for case, results in by_case.items()
            if all(t in results and len(results[t]) == repetitions
                   and all(r["status"] == "verified" and r.get("pipeline_seconds") is not None
                           for r in results[t]) for t in tools)
            and len({r.get("actual_input_sha256") for t in tools for r in results[t]}) == 1}


def write_report(output, metadata, rows):
    tools = metadata["frameworks"]
    data = {"metadata": metadata, "counts": dict(Counter(x["status"] for x in rows)), "results": rows}
    temporary = output / "results.json.tmp"
    temporary.write_text(json.dumps(data, indent=2) + "\n")
    temporary.replace(output / "results.json")
    lines = ["# QLLVM 官方测试集结果", "",
             f"运行时间：{metadata['started_at']}。输入目录共 {metadata['catalog_cases']} 项，本次范围 {metadata['selected_cases']} 项。",
             "完整官方源码已按不可变版本保留；本次运行范围与源码完整性分别统计。", "",
             "| 套件 | 官方版本 | 原始文件数 |", "|---|---|---:|"]
    for suite, source in metadata["sources"].items():
        lines.append(f"| {suite} | {source['reference']} / `{source['revision'][:12]}` | {source['file_count']} |")
    lines += ["", "| 框架 | 版本 | 等价通过 | 仅编译/未验证 | 语义失败 | 编译失败/超时 | 不可用/不支持 | 范围外/未生成 |",
              "|---|---|---:|---:|---:|---:|---:|---:|"]
    for tool in tools:
        mine = [r for r in rows if r["framework"] == tool]
        counts = Counter(r["status"] for r in mine)
        versions = sorted({r["version"] for r in mine if r.get("version")})
        version = ", ".join(versions) or (metadata["compiler_sha256"][:12] if tool == "qllvm" else "未载入")
        lines.append(f"| {tool} | {version} | {counts['verified']} | {counts['compile_only'] + counts['compiled_unverified']} | {counts['semantic_failed']} | {counts['failed'] + counts['timeout']} | {counts['unavailable'] + counts['unsupported']} | {counts['out_of_scope'] + counts['generation_failed']} |")
    lines += ["", "计数按重复运行次数累计；未完成的案例不在以上计数中。运行结束状态：" + str(metadata.get("complete", False)) + "。", "",
              "## 性能对比", "", "以下仅比较所有参测框架都通过同一完整算符检查的相同输入。", "",
              "| 框架 | 共同通过输入数 | 各输入中位耗时的中位数（秒） | 耗时范围（秒） | 同门集门数中位数 | 双比特门中位数 | 深度中位数 |", "|---|---:|---:|---|---:|---:|---:|"]
    paired = paired_samples(rows, tools, metadata["repetitions"])
    for tool in tools:
        times, gates, twoq, depths = [], [], [], []
        for case in paired.values():
            mine = case[tool]
            times.append(statistics.median(r["pipeline_seconds"] for r in mine))
            if all(r.get("common_basis") for group in case.values() for r in group):
                gates.append(statistics.median(r["output_metrics"]["gates"] for r in mine))
                twoq.append(statistics.median(r["output_metrics"]["two_qubit_gates"] for r in mine))
                depths.append(statistics.median(r["output_metrics"]["depth"] for r in mine))
        if times:
            lines.append(f"| {tool} | {len(times)} | {statistics.median(times):.6f} | {min(times):.6f}–{max(times):.6f} | {statistics.median(gates) if gates else '门集不同，不排名'} | {statistics.median(twoq) if twoq else '—'} | {statistics.median(depths) if depths else '—'} |")
        else:
            lines.append(f"| {tool} | 0 | — | — | — | — | — |")
    if "qllvm" in tools and len(tools) > 1:
        lines += ["", "QLLVM 与各框架分别在双方全部重复都通过的同一输入上比较。不同框架的共同集合可能不同，不能横向当作总排名。",
                  "耗时比为对方耗时 / QLLVM耗时，门数比为对方门数 / QLLVM门数；大于1表示该项QLLVM较小。零门电路不计算门数比。", "",
                  "| 对比框架 | 双方通过输入数 | 耗时比几何均值 | 同门集门数比几何均值 |", "|---|---:|---:|---:|"]
        for tool in tools:
            if tool == "qllvm":
                continue
            joint = paired_samples(rows, ["qllvm", tool], metadata["repetitions"])
            timing, gate_ratios = [], []
            for item in joint.values():
                a, b = item["qllvm"], item[tool]
                ta, tb = statistics.median(r["pipeline_seconds"] for r in a), statistics.median(r["pipeline_seconds"] for r in b)
                if ta > 0 and tb > 0:
                    timing.append(tb / ta)
                if all(r.get("common_basis") for r in a+b):
                    ga, gb = statistics.median(r["output_metrics"]["gates"] for r in a), statistics.median(r["output_metrics"]["gates"] for r in b)
                    if ga > 0 and gb > 0:
                        gate_ratios.append(gb / ga)
            def geo(values):
                return f"{math.exp(statistics.mean(math.log(v) for v in values)):.4f}" if values else "—"
            lines.append(f"| {tool} | {len(joint)} | {geo(timing)} | {geo(gate_ratios)} |")
    lines += ["", "- 耗时从读取输入到完成优化及导出；SDK导入、公共输入转换、正确性核验不计时。QLLVM包含原生命令行进程启动。",
              f"- 各编译器使用自身优化器：QLLVM {metadata['opt']}、Qiskit opt3、tket FullPeephole、pyqpanda3 opt2、pyqpanda circuit_optimizer、PennyLane compile。优化强度不假定相等。",
              "- 目标门集为 RX/RY/RZ/CZ/H/X/Y/Z；逐项检查实际输出门集，只有所有输出都符合时才列同门集门数。",
              "- 输入模式：" + metadata["input_mode"] + "。canonical为公共opt0序列化，original为直接官方输入；输入哈希逐项保留。",
              "- 正确性默认比较完整算符（允许独立全局相位）及末端测量接线，不允许将末端经典等价当作可复用量子输出等价。",
              "- 超过核验比特阈值的结果标为仅编译；动态仪器语义不在该算符校验器的能力范围。",
              "- 原生 QLLVM 流程使用 --input-mode original；公平框架对比使用 --input-mode canonical，不能混为一份计时。",
              "- 当前是离线电路编译对比，不包含模拟器执行速度、噪声、硬件、SDK电路构建/修改任务；Benchpress原生任务与QASMBench原生指标另有入口。", "",
              "## 按套件覆盖", "", "| 套件 | 框架 | 等价通过 | 仅编译/未验证 | 其他状态 |", "|---|---|---:|---:|---|"]
    for suite in metadata["sources"]:
        for tool in tools:
            cs = Counter(r["status"] for r in rows if r["suite"] == suite and r["framework"] == tool)
            other = {k: v for k, v in cs.items() if k not in ("verified", "compile_only", "compiled_unverified")}
            lines.append(f"| {suite} | {tool} | {cs['verified']} | {cs['compile_only']+cs['compiled_unverified']} | {json.dumps(other, ensure_ascii=False)} |")
    failures = sorted([r for r in rows if r["status"] not in ("verified", "compile_only")],
                      key=lambda r: r["status"] in ("out_of_scope", "generation_failed"))
    lines += ["", "## 需关注的结果", "", "完整逐项结果见 results.json；每项输入、输出、进程日志和核验结果均保留。", "",
              "| 套件 | 框架 | 案例 | 状态 | 原因 |", "|---|---|---|---|---|"]
    for row in failures[:30]:
        reason = row.get("error") or row.get("verification", {}).get("reason") or str(row.get("verification", {}))
        reason = reason.replace("|", "/").replace("\n", " ")[:180]
        lines.append(f"| {row['suite']} | {row['framework']} | {row['id']} | {row['status']} | {reason} |")
    if len(failures) > 30:
        lines.append(f"\n另有 {len(failures)-30} 条，见 results.json。")
    (output / "summary.md").write_text("\n".join(lines) + "\n")
    if metadata.get("complete"):
        columns = ["suite", "id", "framework", "repetition", "version", "status", "pipeline_seconds",
                   "gates", "two_qubit_gates", "depth", "error"]
        with (output / "results.csv").open("w", newline="") as stream:
            writer = csv.DictWriter(stream, fieldnames=columns)
            writer.writeheader()
            for row in rows:
                values = {k: row.get(k) for k in columns}
                values.update({k: row.get("output_metrics", {}).get(k) for k in ["gates", "two_qubit_gates", "depth"]})
                writer.writerow(values)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=ROOT / "catalog.json")
    parser.add_argument("--frameworks", nargs="+", choices=TOOLS, default=["qllvm"])
    parser.add_argument("--suite", nargs="+", choices=["mqtbench", "qasmbench", "benchpress"])
    parser.add_argument("--profile", choices=["all", "smoke"], default="all")
    parser.add_argument("--smoke-per-suite", type=int, default=6)
    parser.add_argument("--case", nargs="+")
    parser.add_argument("--python", type=Path, default=Path(sys.executable))
    parser.add_argument("--library-preload", type=Path, help="Optional process-local shared library override, recorded in results")
    parser.add_argument("--compiler", type=Path, default=ROOT.parent.parent / "build/bin/qllvm-compile")
    parser.add_argument("--input-mode", choices=["original", "canonical"], default="original")
    parser.add_argument("--opt", choices=["O0", "O1"], default="O1")
    parser.add_argument("--repetitions", type=int, default=3)
    parser.add_argument("--warmups", type=int, default=1)
    parser.add_argument("--timeout", type=float, default=60)
    parser.add_argument("--memory-gb", type=float, default=8)
    parser.add_argument("--max-qubits", type=int, default=0, help="0 means all; excluded inputs stay visible")
    parser.add_argument("--max-bytes", type=int, default=0, help="0 means all; excluded inputs stay visible")
    parser.add_argument("--verify-qubits", type=int, default=6)
    parser.add_argument("--tolerance", type=float, default=1e-7)
    parser.add_argument("--require-verification", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--resume", action="store_true")
    parser.add_argument("--list", action="store_true")
    args = parser.parse_args()
    if args.repetitions < 1 or args.warmups < 0 or args.timeout <= 0 or not math.isfinite(args.timeout):
        parser.error("Invalid repetition/warmup/timeout")
    if not 1 <= args.verify_qubits <= 8 or not 0 < args.tolerance <= 1e-3:
        parser.error("Verification requires 1..8 qubits and 0<tolerance<=1e-3")
    if len(set(args.frameworks)) != len(args.frameworks):
        parser.error("Duplicate frameworks")
    if args.smoke_per_suite < 1 or args.max_qubits < 0 or args.max_bytes < 0 or args.memory_gb < 0 or not math.isfinite(args.memory_gb):
        parser.error("Invalid scope or memory limits")
    if len(args.frameworks) > 1 and args.input_mode != "canonical":
        parser.error("Cross-framework comparison requires --input-mode canonical")
    catalog = json.loads(args.catalog.read_text())
    versions = json.loads((ROOT / "versions.json").read_text())
    for name in catalog["sources"]:
        verify(name)
    cases = choose_cases(catalog, args)
    if not cases:
        parser.error("No cases selected")
    if args.list:
        print(json.dumps(cases, indent=2)); return 0
    args.output = args.output.resolve()
    # A venv's python is often a symlink. Resolving that symlink changes
    # sys.prefix and silently drops its installed SDKs in worker processes.
    args.python = Path(os.path.abspath(args.python))
    if not args.python.is_file():
        parser.error("Python executable does not exist")
    args.compiler = args.compiler.resolve(strict=True)
    args.output.mkdir(parents=True, exist_ok=args.resume)
    config = {"catalog_sha256": sha(args.catalog), "frameworks": args.frameworks,
              "profile": args.profile,
              "input_mode": args.input_mode, "opt": args.opt, "repetitions": args.repetitions,
              "warmups": args.warmups, "timeout": args.timeout, "memory_gb": args.memory_gb,
              "max_qubits": args.max_qubits, "max_bytes": args.max_bytes,
              "verify_qubits": args.verify_qubits, "tolerance": args.tolerance,
              "require_verification": args.require_verification,
              "selected_ids": [c["id"] for c in cases], "python": str(args.python),
              "library_preload": str(args.library_preload) if args.library_preload else None,
              "compiler_sha256": sha(args.compiler)}
    config["versions_sha256"] = sha(ROOT / "versions.json")
    metadata = {**config, "started_at": datetime.now(timezone.utc).isoformat(),
                "sources": catalog["sources"], "catalog_cases": len(catalog["cases"]),
                "selected_cases": len(cases), "host": platform.platform(), "cpu": platform.processor(),
                "cpu_count": os.cpu_count(), "threads_per_worker": 1,
                "command": sys.argv, "runner_sha256": sha(Path(__file__).resolve()),
                "worker_sha256": sha(ROOT / "worker.py"), "complete": False}
    metadata["expected_versions"] = versions
    repo = ROOT.parent.parent
    metadata["source_revision"] = subprocess.check_output(["git", "-C", str(repo), "rev-parse", "HEAD"], text=True).strip()
    metadata["source_worktree_status"] = subprocess.check_output(["git", "-C", str(repo), "status", "--porcelain"], text=True).splitlines()
    metadata["binary_provenance_note"] = "Binary hash identifies this executable; recorded working tree is not proof the binary was rebuilt from it"
    if args.resume:
        previous = json.loads((args.output / "results.json").read_text())
        for key, value in config.items():
            if previous["metadata"].get(key) != value:
                parser.error("Resume configuration changed: " + key)
        if previous["metadata"].get("worker_sha256") != metadata["worker_sha256"]:
            parser.error("Worker changed; use a fresh output")
        if previous["metadata"].get("runner_sha256") != metadata["runner_sha256"]:
            parser.error("Runner changed; use a fresh output")
        metadata = previous["metadata"]
        rows = previous["results"]
    else:
        rows = []
    done_ids = {(r["id"], r["framework"], r["repetition"]) for r in rows}
    base_request = {"compiler": str(args.compiler), "opt": args.opt,
                    "verify_qubits": args.verify_qubits, "tolerance": args.tolerance}
    write_report(args.output, metadata, rows)
    for index, case in enumerate(cases):
        if all((case["id"], tool, repetition) in done_ids for tool in args.frameworks for repetition in range(1, args.repetitions+1)):
            continue
        path = safe_path(case["file"]) if case.get("file") else None
        reason = None
        status = "out_of_scope"
        if case["generation_status"] != "ready":
            status, reason = "generation_failed", case.get("error", "Generator failed")
        elif args.max_qubits and (case.get("qubits") is None or case["qubits"] > args.max_qubits):
            reason = "Explicit --max-qubits limit (or width unknown)"
        elif args.max_bytes and case.get("bytes", path.stat().st_size) > args.max_bytes:
            reason = "Explicit --max-bytes limit"
        elif sha(path) != case["sha256"]:
            raise ValueError("Input checksum mismatch: " + str(path))
        case_dir = args.output / case["id"]
        if not reason and case.get("archive_member"):
            extracted = case_dir / "archive/input.qasm"
            extracted.parent.mkdir(parents=True, exist_ok=True)
            if not extracted.exists():
                with tarfile.open(path) as archive:
                    member = archive.getmember(case["archive_member"])
                    if not member.isfile():
                        raise ValueError("Archived input is not a regular file")
                    with archive.extractfile(member) as stream, extracted.open("wb") as destination:
                        shutil.copyfileobj(stream, destination)
            path = extracted
        canonical = case_dir / "prepare/compiled.qasm"
        prep_record = case_dir / "prepared.json"
        prepared = False
        if prep_record.exists():
            prep_info = json.loads(prep_record.read_text())
            canonical = args.output / prep_info["path"]
            prepared = canonical.exists() and sha(canonical) == prep_info["sha256"]
            if not prepared:
                raise ValueError("Previously prepared input changed: " + str(canonical))
        if not reason and args.input_mode == "canonical" and not prepared:
            prep = task(args, {**base_request, "framework": "prepare", "input": str(path)},
                        case_dir / "prepare")
            if prep["status"] != "prepared":
                status, reason = "unsupported", "Shared input serialization failed: " + prep.get("error", prep["status"])
            else:
                canonical = args.output / prep["artifacts"] / "compiled.qasm"
                prep_record.write_text(json.dumps({"path": str(canonical.relative_to(args.output)), "sha256": sha(canonical)}, indent=2) + "\n")
        actual_input = canonical if args.input_mode == "canonical" else path
        for repetition in range(1, args.repetitions + 1):
            # Deterministic rotation avoids always measuring QLLVM first.
            order = args.frameworks[(index+repetition) % len(args.frameworks):] + args.frameworks[:(index+repetition) % len(args.frameworks)]
            for tool in order:
                if (case["id"], tool, repetition) in done_ids:
                    continue
                row = {"id": case["id"], "suite": case["suite"], "family": case["family"],
                       "level": case["level"], "framework": tool, "repetition": repetition,
                       "input_sha256": case.get("sha256")}
                if reason:
                    row.update(status=status, error=reason)
                else:
                    package = "pytket" if tool == "tket" else tool
                    expected = versions["packages"].get(package, {}).get("version")
                    request = {**base_request, "framework": tool, "input": str(actual_input),
                               "expected_version": expected}
                    if repetition == 1:
                        for warmup in range(args.warmups):
                            task(args, dict(request), case_dir / tool / ("warmup-" + str(warmup)))
                    result = task(args, request, case_dir / tool / str(repetition))
                    row.update(result)
                    row["actual_input_sha256"] = sha(actual_input)
                rows.append(row)
                write_report(args.output, metadata, rows)
                if not reason:
                    print(index+1, "/", len(cases), tool, repetition, case["id"], row["status"], flush=True)
    metadata["complete"] = True
    write_report(args.output, metadata, rows)
    print(json.dumps({"report": str(args.output / "summary.md"), "counts": dict(Counter(r["status"] for r in rows))}))
    bad = ("failed", "timeout", "semantic_failed", "unavailable", "generation_failed")
    return 1 if any(r["status"] in bad or (args.require_verification and r["status"] not in ("verified", "out_of_scope")) for r in rows) else 0


if __name__ == "__main__":
    sys.exit(main())
