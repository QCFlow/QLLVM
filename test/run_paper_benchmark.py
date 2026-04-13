#!/usr/bin/env python3
# This code is part of QLLVM.
#
# (C) Copyright QCFlow 2026.
#
# This code is licensed under the Apache License, Version 2.0.

"""
Full MQTBench compiler benchmark with checkpointing + paper-ready exports.

论文中可这样组织（建议）：
  1) 数据集：说明来源（MQTBench）、电路数量、文件命名约定；本文统计 N 个 OpenQASM 2.0 电路。
  2) 主表（Table）：benchmark_summary.csv — 各编译器在 gate / depth / time / memory 上的
     均值、中位数、标准差（成功样本数 n）。
  3) 附表 / 补充材料：benchmark_long.csv — 长表（每行：电路 × 编译器），便于审稿人复现。
  4) 图：fig1 汇总柱状图；fig2 相对 Qiskit 的门数比分布；fig3 QLLVM vs Qiskit 散点；
     fig4 编译时间箱线图。图中注明：Cirq / PennyLane 与 Qiskit 基组与优化策略不同，门数/深度为趋势对比。

用法：
  conda activate qllvm
  cd test
  python run_paper_benchmark.py [--max-files N] [--no-resume] [--plot-only]

默认输出目录：test/paper_benchmark_out/
"""

from __future__ import annotations

import argparse
import csv
import os
import sys
from time import perf_counter

# 保证可从 test/ 目录运行
_TEST_DIR = os.path.dirname(os.path.abspath(__file__))
if _TEST_DIR not in sys.path:
    sys.path.insert(0, _TEST_DIR)

from utils.benchmark_compilers import benchmark_one_file, results_to_rows
from utils.benchmark_paper_plots import load_long_csv, plot_all, write_summary_csv
from utils.get_qasm_file import get_qasm_files

CSV_FIELDS = [
    "backend",
    "qasm_path",
    "file",
    "gate_count",
    "depth",
    "compile_time_s",
    "memory_peak_bytes",
    "memory_peak_MiB",
    "error",
    "notes",
]


def _default_qllvm_bin() -> str:
    c = os.path.expanduser("~/.qllvm/bin/qllvm")
    return c if os.path.isfile(c) else "qllvm"


def _load_done_keys(csv_path: str) -> set[tuple[str, str]]:
    """Completed (absolute_path, backend) pairs."""
    if not os.path.isfile(csv_path):
        return set()
    import pandas as pd

    df = pd.read_csv(csv_path)
    done = set()
    for _, row in df.iterrows():
        p = os.path.abspath(str(row["qasm_path"]))
        b = str(row["backend"])
        if pd.isna(row.get("error")) or str(row.get("error")) == "nan":
            done.add((p, b))
    return done


def run_benchmark(
    mqtbench_dir: str,
    out_dir: str,
    *,
    max_files: int | None,
    resume: bool,
    qllvm_bin: str,
) -> str:
    os.makedirs(out_dir, exist_ok=True)
    long_csv = os.path.join(out_dir, "benchmark_long.csv")
    err_csv = os.path.join(out_dir, "benchmark_errors.csv")

    files = get_qasm_files(mqtbench_dir)
    if max_files is not None:
        files = files[:max_files]

    done = _load_done_keys(long_csv) if resume else set()
    file_exists = os.path.isfile(long_csv) and os.path.getsize(long_csv) > 0
    if not resume and file_exists:
        os.rename(long_csv, long_csv + ".bak")
        file_exists = False

    append = resume and file_exists
    mode = "a" if append else "w"
    write_header = not append
    t0 = perf_counter()

    with open(long_csv, mode, newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=CSV_FIELDS, extrasaction="ignore")
        if write_header:
            writer.writeheader()

        for i, qasm_path in enumerate(files):
            ap = os.path.abspath(qasm_path)
            backends = ["qllvm", "qiskit", "cirq", "pennylane"]
            need = [b for b in backends if (ap, b) not in done]
            if not need:
                continue

            print(f"[{i+1}/{len(files)}] {os.path.basename(qasm_path)} (backends: {need})", flush=True)  # noqa: E501

            results = benchmark_one_file(
                qasm_path,
                backends=need,
                qllvm_bin=qllvm_bin,
            )
            for row in results_to_rows(results):
                row["file"] = os.path.basename(row["qasm_path"])
                writer.writerow(row)
            f.flush()

    elapsed = perf_counter() - t0
    print(f"Finished benchmark loop in {elapsed:.1f}s. Long table: {long_csv}")

    # 错误汇总
    import pandas as pd

    df = pd.read_csv(long_csv)
    bad = df[df["error"].notna() & (df["error"].astype(str).str.len() > 0)]
    if len(bad):
        bad.to_csv(err_csv, index=False)
        print(f"Recorded {len(bad)} rows with errors -> {err_csv}")
    else:
        if os.path.isfile(err_csv):
            os.remove(err_csv)

    write_summary_csv(df, os.path.join(out_dir, "benchmark_summary.csv"))
    print(f"Summary -> {os.path.join(out_dir, 'benchmark_summary.csv')}")

    return long_csv


def main() -> int:
    parser = argparse.ArgumentParser(description="MQTBench paper benchmark")
    parser.add_argument(
        "--mqtbench",
        default=os.path.join(_TEST_DIR, "MQTBench"),
        help="Folder containing .qasm files",
    )
    parser.add_argument(
        "--out",
        default=os.path.join(_TEST_DIR, "paper_benchmark_out"),
        help="Output directory",
    )
    parser.add_argument(
        "--max-files",
        type=int,
        default=None,
        help="Limit number of circuits (debug)",
    )
    parser.add_argument(
        "--no-resume",
        action="store_true",
        help="Ignore checkpoint and start fresh (renames old CSV)",
    )
    parser.add_argument(
        "--plot-only",
        action="store_true",
        help="Only read benchmark_long.csv and write figures",
    )
    parser.add_argument("--qllvm-bin", default=_default_qllvm_bin())
    args = parser.parse_args()

    long_csv = os.path.join(args.out, "benchmark_long.csv")

    if args.plot_only:
        if not os.path.isfile(long_csv):
            print(f"Missing {long_csv}", file=sys.stderr)
            return 1
        paths = plot_all(long_csv, args.out, dpi=200)
        print("Generated:", *paths, sep="\n  ")
        return 0

    run_benchmark(
        args.mqtbench,
        args.out,
        max_files=args.max_files,
        resume=not args.no_resume,
        qllvm_bin=args.qllvm_bin,
    )

    if os.path.isfile(long_csv):
        paths = plot_all(long_csv, args.out, dpi=200)
        print("Figures:", *paths, sep="\n  ")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
