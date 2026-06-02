#!/usr/bin/env python3
"""Figures for MPI hybrid experiments (Exp-1 build scalability)."""
from __future__ import annotations

import csv
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parent
CSV = ROOT / "results" / "mpi_build_scalability.csv"
FIG = ROOT / "results" / "figures"

plt.rcParams.update(
    {
        "font.family": "serif",
        "font.size": 9,
        "axes.labelsize": 9,
        "axes.titlesize": 9,
        "legend.fontsize": 8,
        "xtick.labelsize": 8,
        "ytick.labelsize": 8,
        "lines.linewidth": 1.2,
        "lines.markersize": 5,
    }
)


def _style_axes(ax):
    ax.grid(True, alpha=0.25, linewidth=0.5)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)


def main() -> int:
    if not CSV.is_file():
        print(f"Missing {CSV}; run run_mpi_experiments.py first", flush=True)
        return 1
    FIG.mkdir(parents=True, exist_ok=True)
    with CSV.open() as f:
        rows = [r for r in csv.DictReader(f) if r["experiment"] == "mpi_kernel_count"]

    k = np.array([int(r["K"]) for r in rows])
    t_u = np.array([float(r["T_build_unified_mpi_s"]) for r in rows])
    t_s = np.array([float(r["T_build_split_mpi_s"]) for r in rows])

    fig, ax = plt.subplots(figsize=(3.4, 2.6))
    ax.plot(k, t_u, "o-", label="QLLVM unified (-mpi)", color="#1f77b4")
    ax.plot(k, t_s, "s--", label="MPI split pipeline", color="#d62728")
    ax.set_xlabel(r"Number of quantum kernels $K$")
    ax.set_ylabel(r"End-to-end build time $T_{\mathrm{build}}$ (s)")
    ax.set_xticks(k)
    ymax = max(float(t_s.max()), float(t_u.max())) * 1.15
    ax.set_ylim(0, ymax)
    ax.legend(loc="upper left", frameon=False)
    _style_axes(ax)

    fig.tight_layout()
    out = FIG / "fig_mpi_build_scalability"
    fig.savefig(out.with_suffix(".pdf"), bbox_inches="tight")
    fig.savefig(out.with_suffix(".png"), dpi=300, bbox_inches="tight")
    plt.close(fig)
    print(out.with_suffix(".pdf"))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
