# This code is part of QLLVM.
#
# (C) Copyright QCFlow 2026.
#
# This code is licensed under the Apache License, Version 2.0.

"""
Publication-style figures from benchmark_long.csv (see run_paper_benchmark.py).
"""

from __future__ import annotations

import os
from typing import Optional

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

BACKEND_ORDER = ["qllvm", "qiskit", "cirq", "pennylane"]
BACKEND_COLORS = {
    "qllvm": "#1f77b4",
    "qiskit": "#ff7f0e",
    "cirq": "#2ca02c",
    "pennylane": "#d62728",
}
BACKEND_LABELS = {
    "qllvm": "QLLVM",
    "qiskit": "Qiskit",
    "cirq": "Cirq",
    "pennylane": "PennyLane",
}


def load_long_csv(path: str) -> pd.DataFrame:
    df = pd.read_csv(path)
    df["file"] = df["qasm_path"].apply(lambda p: os.path.basename(str(p)))
    return df


def _summary_table(df: pd.DataFrame) -> pd.DataFrame:
    ok = df[df["error"].isna() & df["gate_count"].notna()].copy()
    return (
        ok.groupby("backend")
        .agg(
            n=("file", "nunique"),
            gate_mean=("gate_count", "mean"),
            gate_median=("gate_count", "median"),
            gate_std=("gate_count", "std"),
            depth_mean=("depth", "mean"),
            depth_median=("depth", "median"),
            time_mean_s=("compile_time_s", "mean"),
            time_median_s=("compile_time_s", "median"),
            mem_mean_MiB=("memory_peak_MiB", "mean"),
            mem_median_MiB=("memory_peak_MiB", "median"),
        )
        .reindex([b for b in BACKEND_ORDER if b in ok["backend"].unique()])
    )


def plot_summary_bars(
    df: pd.DataFrame,
    out_path: str,
    *,
    dpi: int = 200,
) -> None:
    """Bar chart: mean gate count, depth, compile time, peak memory per backend."""
    summ = _summary_table(df)
    if summ.empty:
        raise ValueError("No successful rows for plotting")

    backends = [b for b in BACKEND_ORDER if b in summ.index]
    labels = [BACKEND_LABELS.get(b, b) for b in backends]
    colors = [BACKEND_COLORS.get(b, "#333") for b in backends]

    fig, axes = plt.subplots(2, 2, figsize=(10, 8))
    fig.suptitle("Compiler benchmark — aggregate means (successful runs)", fontsize=12)

    x = np.arange(len(backends))
    w = 0.65

    ax = axes[0, 0]
    vals = [summ.loc[b, "gate_mean"] for b in backends]
    ax.bar(x, vals, width=w, color=colors, edgecolor="black", linewidth=0.3)
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=15, ha="right")
    ax.set_ylabel("Mean gate count")
    ax.set_title("Compiled circuit size")

    ax = axes[0, 1]
    vals = [summ.loc[b, "depth_mean"] for b in backends]
    ax.bar(x, vals, width=w, color=colors, edgecolor="black", linewidth=0.3)
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=15, ha="right")
    ax.set_ylabel("Mean depth")
    ax.set_title("Circuit depth")

    ax = axes[1, 0]
    vals = [summ.loc[b, "time_mean_s"] for b in backends]
    ax.bar(x, vals, width=w, color=colors, edgecolor="black", linewidth=0.3)
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=15, ha="right")
    ax.set_ylabel("Mean time (s)")
    ax.set_title("Compile time")

    ax = axes[1, 1]
    vals = [summ.loc[b, "mem_mean_MiB"] for b in backends]
    ax.bar(x, vals, width=w, color=colors, edgecolor="black", linewidth=0.3)
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=15, ha="right")
    ax.set_ylabel("Mean peak RSS (MiB)")
    ax.set_title("Peak memory (Python: tracemalloc; QLLVM: subprocess RSS)")

    fig.tight_layout()
    fig.savefig(out_path, dpi=dpi, bbox_inches="tight")
    plt.close(fig)


def plot_gate_ratio_vs_qiskit(
    df: pd.DataFrame,
    out_path: str,
    *,
    dpi: int = 200,
) -> None:
    """Boxplot: gate_count / qiskit_gate per circuit (only circuits where both OK)."""
    ok = df[df["error"].isna() & df["gate_count"].notna()].copy()
    wide = ok.pivot_table(
        index="file",
        columns="backend",
        values="gate_count",
        aggfunc="first",
    )
    if "qiskit" not in wide.columns:
        return
    ratios = {}
    for b in BACKEND_ORDER:
        if b == "qiskit" or b not in wide.columns:
            continue
        r = wide[b] / wide["qiskit"]
        r = r.replace([np.inf, -np.inf], np.nan).dropna()
        if len(r) > 0:
            ratios[b] = r.values

    if not ratios:
        return

    fig, ax = plt.subplots(figsize=(8, 5))
    positions = range(1, len(ratios) + 1)
    bp = ax.boxplot(
        [ratios[k] for k in ratios],
        positions=list(positions),
        widths=0.5,
        patch_artist=True,
        showfliers=False,
    )
    for patch, b in zip(bp["boxes"], ratios.keys()):
        patch.set_facecolor(BACKEND_COLORS.get(b, "#ccc"))
        patch.set_alpha(0.85)
    ax.axhline(1.0, color="gray", linestyle="--", linewidth=1, label="Qiskit baseline")
    ax.set_xticks(list(positions))
    ax.set_xticklabels([BACKEND_LABELS.get(b, b) for b in ratios.keys()])
    ax.set_ylabel("Gate count ratio vs Qiskit")
    ax.set_title("Distribution of gate-count ratios (per circuit)")
    fig.tight_layout()
    fig.savefig(out_path, dpi=dpi, bbox_inches="tight")
    plt.close(fig)


def plot_scatter_qllvm_vs_qiskit(
    df: pd.DataFrame,
    out_path: str,
    *,
    dpi: int = 200,
) -> None:
    ok = df[df["error"].isna() & df["gate_count"].notna()].copy()
    wide = ok.pivot_table(
        index="file",
        columns="backend",
        values="gate_count",
        aggfunc="first",
    )
    if "qiskit" not in wide.columns or "qllvm" not in wide.columns:
        return
    x = wide["qiskit"].values
    y = wide["qllvm"].values
    m = np.isfinite(x) & np.isfinite(y)
    x, y = x[m], y[m]
    if len(x) == 0:
        return

    fig, ax = plt.subplots(figsize=(6, 6))
    ax.scatter(x, y, s=12, alpha=0.5, c=BACKEND_COLORS["qllvm"], edgecolors="none")
    lim = max(0, float(np.nanmax(np.concatenate([x, y]))) * 1.05)
    ax.plot([0, lim], [0, lim], "k--", linewidth=1, label="y = x")
    ax.set_xlabel("Qiskit gate count")
    ax.set_ylabel("QLLVM gate count")
    ax.set_title("Compiled gate count: QLLVM vs Qiskit (per circuit)")
    ax.set_aspect("equal", adjustable="box")
    ax.legend()
    fig.tight_layout()
    fig.savefig(out_path, dpi=dpi, bbox_inches="tight")
    plt.close(fig)


def plot_compile_time_box(
    df: pd.DataFrame,
    out_path: str,
    *,
    dpi: int = 200,
) -> None:
    ok = df[df["error"].isna() & df["compile_time_s"].notna()].copy()
    data = []
    labels = []
    for b in BACKEND_ORDER:
        sub = ok[ok["backend"] == b]["compile_time_s"]
        if len(sub) > 0:
            data.append(sub.values)
            labels.append(BACKEND_LABELS.get(b, b))

    if not data:
        return

    fig, ax = plt.subplots(figsize=(8, 5))
    bp = ax.boxplot(
        data,
        labels=labels,
        patch_artist=True,
        showfliers=False,
    )
    used = [b for b in BACKEND_ORDER if b in ok["backend"].unique()]
    for patch, b in zip(bp["boxes"], used):
        patch.set_facecolor(BACKEND_COLORS.get(b, "#ccc"))
        patch.set_alpha(0.85)
    ax.set_ylabel("Compile time (s)")
    ax.set_title("Compile time distribution (per circuit)")
    plt.xticks(rotation=15, ha="right")
    fig.tight_layout()
    fig.savefig(out_path, dpi=dpi, bbox_inches="tight")
    plt.close(fig)


def plot_all(
    csv_path: str,
    out_dir: str,
    *,
    dpi: int = 200,
) -> list[str]:
    """Generate all figures; return list of written paths."""
    df = load_long_csv(csv_path)
    os.makedirs(out_dir, exist_ok=True)
    out: list[str] = []

    p1 = os.path.join(out_dir, "fig1_summary_bars.png")
    plot_summary_bars(df, p1, dpi=dpi)
    out.append(p1)

    p2 = os.path.join(out_dir, "fig2_gate_ratio_vs_qiskit.png")
    plot_gate_ratio_vs_qiskit(df, p2, dpi=dpi)
    out.append(p2)

    p3 = os.path.join(out_dir, "fig3_scatter_qllvm_vs_qiskit_gates.png")
    plot_scatter_qllvm_vs_qiskit(df, p3, dpi=dpi)
    out.append(p3)

    p4 = os.path.join(out_dir, "fig4_compile_time_box.png")
    plot_compile_time_box(df, p4, dpi=dpi)
    out.append(p4)

    return out


def write_summary_csv(df: pd.DataFrame, path: str) -> None:
    summ = _summary_table(df)
    summ.to_csv(path)
