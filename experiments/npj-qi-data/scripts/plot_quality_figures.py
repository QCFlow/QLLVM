"""Redraw Figure 5: color by above/below the equal-metric diagonal."""
from pathlib import Path
import csv
import shutil
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from matplotlib.ticker import LogLocator, NullFormatter, LogFormatterMathtext

import argparse
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--data', type=Path, default=Path(__file__).resolve().parent/'data/paired_quality.csv')
parser.add_argument('--output-dir', type=Path, default=Path(__file__).resolve().parents[1]/'Figures')
args = parser.parse_args()
DATA = args.data
FIG = args.output_dir
FIG.mkdir(parents=True, exist_ok=True)
RED = "#d62728"
BLUE = "#1f77b4"
GRAY = "#858585"


plt.rcParams.update({
    "font.family": "Liberation Sans",
    "font.size": 8,
    "axes.labelsize": 8.4,
    "xtick.labelsize": 7.6,
    "ytick.labelsize": 7.6,
    "legend.fontsize": 7,
    "axes.titlesize": 8.3,
    "axes.titleweight": "normal",
    "axes.linewidth": 0.65,
    "xtick.direction": "out",
    "ytick.direction": "out",
    "xtick.major.width": 0.65,
    "ytick.major.width": 0.65,
    "xtick.major.size": 2.7,
    "ytick.major.size": 2.7,
    "xtick.minor.width": 0.45,
    "ytick.minor.width": 0.45,
    "xtick.minor.size": 1.5,
    "ytick.minor.size": 1.5,
    "axes.grid": False,
    "axes.spines.top": True,
    "axes.spines.right": True,
    "pdf.fonttype": 42,
    "ps.fonttype": 42,
    "svg.fonttype": "none",
    "mathtext.fontset": "dejavusans",
    "savefig.facecolor": "white",
    "figure.facecolor": "white",
    "axes.facecolor": "white",
})

quality = list(csv.DictReader(DATA.open()))
assert len(quality) == 430

limits = {}
for metric in ["gate_count", "depth"]:
    vals = np.array([int(r[e + "_" + metric]) for r in quality for e in ["qllvm", "qiskit", "cirq"]])
    assert np.all(vals > 0)
    limits[metric] = (10 ** (np.log10(vals.min()) - 0.14), 10 ** (np.log10(vals.max()) + 0.13))

legend = [
    Line2D([], [], ls="none", marker="o", ms=3.2, mfc=BLUE, mec=BLUE, label="QLLVM lower"),
    Line2D([], [], ls="none", marker="o", ms=3.2, mfc=RED, mec=RED, label="QLLVM higher"),
    Line2D([], [], ls="none", marker="o", ms=3.2, mfc=GRAY, mec=GRAY, label="Equal"),
]


def label(ax, s):
    ax.text(-0.18, 1.02, s, transform=ax.transAxes, ha="left", va="bottom",
            fontsize=9, fontweight="bold", color="black")


def panel(ax, wf, metric, letter, showlegend=False):
    x = np.array([int(r[wf + "_" + metric]) for r in quality])
    y = np.array([int(r["qllvm_" + metric]) for r in quality])
    below = y < x
    above = y > x
    equal = y == x
    unit = "gates" if metric == "gate_count" else "depth"
    title = "Qiskit" if wf == "qiskit" else "Cirq control"
    ax.set_xscale("log")
    ax.set_yscale("log")
    lo, hi = limits[metric]
    ax.set(xlim=(lo, hi), ylim=(lo, hi), xlabel=title + " " + unit, ylabel="QLLVM " + unit)
    ax.plot([lo, hi], [lo, hi], color=GRAY, lw=0.8, ls=(0, (4, 3)), zorder=1)
    ax.scatter(x[below], y[below], s=8, color=BLUE, alpha=0.72, linewidths=0, zorder=3)
    ax.scatter(x[above], y[above], s=8, color=RED, alpha=0.72, linewidths=0, zorder=3)
    ax.scatter(x[equal], y[equal], s=8, color=GRAY, alpha=0.72, linewidths=0, zorder=3)
    for axis in [ax.xaxis, ax.yaxis]:
        axis.set_major_locator(LogLocator(base=10, numticks=5))
        axis.set_major_formatter(LogFormatterMathtext())
        axis.set_minor_locator(LogLocator(base=10, subs=[2, 5], numticks=15))
        axis.set_minor_formatter(NullFormatter())
    label(ax, letter)
    if showlegend:
        ax.legend(handles=legend, loc="upper left", frameon=False, borderpad=0.05,
                  handletextpad=0.35, labelspacing=0.2, fontsize=6.5)
    print(f"{wf} {metric}: below={below.sum()} above={above.sum()} equal={equal.sum()}")


for wf, letters, title in [
    ("qiskit", ["a", "c"], "Qiskit"),
    ("cirq", ["b", "d"], "Cirq basis-normalization control"),
]:
    fig, axs = plt.subplots(2, 1, figsize=(3.45, 4.25))
    fig.subplots_adjust(left=0.18, right=0.97, bottom=0.12, top=0.93, hspace=0.52)
    for i, (metric, letter) in enumerate(zip(["gate_count", "depth"], letters)):
        panel(axs[i], wf, metric, letter, i == 0)
    axs[0].set_title(title, pad=7)
    out = FIG / f"quality_{wf}_final.pdf"
    fig.savefig(out, dpi=320)
    fig.savefig(out.with_suffix(".png"), dpi=320)
    plt.close(fig)
    print("wrote", out)
