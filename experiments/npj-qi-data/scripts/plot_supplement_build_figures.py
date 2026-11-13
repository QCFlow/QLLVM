"""Redraw supplementary S4/S1 with the main-text Figure 3 palette."""
from pathlib import Path
import csv
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator

import argparse
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--data', type=Path, required=True, help='Original build_summary.csv; not supplied in this package')
parser.add_argument('--output-dir', type=Path, default=Path(__file__).resolve().parents[1]/'Figures')
args = parser.parse_args()
CSV = args.data
FIG = args.output_dir
if not CSV.is_file():
    parser.error('Provide the original build_summary.csv; it is not included in this package.')
FIG.mkdir(parents=True, exist_ok=True)
RED = '#d62728'
BLUE = '#1f77b4'
GRAY = '#777777'
WORKFLOWS = [
    ('make', 'Parallel Make', GRAY, '^'),
    ('cmake_ninja', 'CMake/Ninja', BLUE, 's'),
    ('qllvm_specialize', 'QLLVM + specialization', RED, 'o'),
]

plt.rcParams.update({
    'font.family': 'Liberation Sans',
    'font.size': 8,
    'axes.labelsize': 8,
    'xtick.labelsize': 7.5,
    'ytick.labelsize': 7.5,
    'legend.fontsize': 7.2,
    'axes.titlesize': 8.2,
    'axes.linewidth': 0.65,
    'xtick.direction': 'out',
    'ytick.direction': 'out',
    'xtick.major.width': 0.65,
    'ytick.major.width': 0.65,
    'xtick.major.size': 2.7,
    'ytick.major.size': 2.7,
    'axes.grid': False,
    'pdf.fonttype': 42,
    'ps.fonttype': 42,
    'svg.fonttype': 'none',
    'mathtext.fontset': 'dejavusans',
    'savefig.facecolor': 'white',
    'figure.facecolor': 'white',
    'axes.facecolor': 'white',
})

rows = list(csv.DictReader(CSV.open()))


def get(k, qubits, layers, cores, workflow):
    for r in rows:
        if (r['K'] == str(k) and r['qubits'] == str(qubits) and r['layers'] == str(layers)
                and r['cores'] == str(cores) and r['cuda'] == 'True' and r['workflow'] == workflow):
            return r
    raise KeyError((k, qubits, layers, cores, workflow))


def values(points, workflow, field):
    return [float(get(*pt, workflow)[field]) for pt in points]


def yerr(points, workflow, mean_field, lo_field, hi_field):
    means = values(points, workflow, mean_field)
    los = values(points, workflow, lo_field)
    his = values(points, workflow, hi_field)
    return means, [[m - lo for m, lo in zip(means, los)], [hi - m for m, hi in zip(means, his)]]


def plot_lines(ax, xs, points, mean_field, lo_field, hi_field, logx=False):
    handles = []
    for z, (workflow, label, color, marker) in enumerate(WORKFLOWS, start=2):
        means, err = yerr(points, workflow, mean_field, lo_field, hi_field)
        h = ax.errorbar(
            xs, means, yerr=err, color=color, marker=marker, linestyle='-',
            label=label, markersize=3.6, linewidth=0.95, elinewidth=0.7,
            capsize=2, mew=0.7, zorder=z,
        )
        handles.append(h)
    if logx:
        ax.set_xscale('log', base=2)
    return handles


def letter(ax, text, x=-0.20):
    ax.text(x, 1.08, text, transform=ax.transAxes, fontsize=10, fontweight='bold', va='bottom')


# --- Figure S4 ---
cores = [1, 2, 4, 8, 16, 20, 40]
ks = [1, 2, 4, 8, 16, 32, 64, 128, 256]
kp = [k / 8.0 for k in ks]

fig, axs = plt.subplots(2, 3, figsize=(7.05, 5.70))
fig.subplots_adjust(left=0.078, right=0.995, top=0.915, bottom=0.125, wspace=0.42, hspace=0.48)

core_pts = [(32, 16, 4, p) for p in cores]
plot_lines(axs[0, 0], cores, core_pts, 'mean_s', 'ci95_low_s', 'ci95_high_s')
axs[0, 0].set(xlabel='Physical cores P', ylabel='Build time (s)', title='Fixed $K=U=32$, 16 qubits')
letter(axs[0, 0], 'a')
axs[0, 0].set_xlim(0, 42)
axs[0, 0].yaxis.set_major_locator(MaxNLocator(nbins=6))

plot_lines(axs[0, 1], cores, core_pts, 'mean_cpu_s', 'cpu_ci95_low_s', 'cpu_ci95_high_s')
axs[0, 1].set(xlabel='Physical cores P', ylabel='Aggregate CPU time (s)', title='CPU work at fixed $K$')
letter(axs[0, 1], 'b')
axs[0, 1].set_xlim(0, 42)

# Strong scaling T(1)/T(P)
handles = []
for z, (workflow, label, color, marker) in enumerate(WORKFLOWS, start=2):
    t1 = float(get(32, 16, 4, 1, workflow)['mean_s'])
    ys = [t1 / float(get(32, 16, 4, p, workflow)['mean_s']) for p in cores]
    h = axs[0, 2].plot(cores, ys, color=color, marker=marker, markersize=3.6, linewidth=0.95, label=label, zorder=z)
    handles.append(h[0])
axs[0, 2].set(xlabel='Physical cores P', ylabel='Speedup vs 1-core time', title='Strong scaling', xlim=(0, 42))
letter(axs[0, 2], 'c')

inst_pts = [(k, 16, 4, 8) for k in ks]
plot_lines(axs[1, 0], kp, inst_pts, 'mean_s', 'ci95_low_s', 'ci95_high_s', logx=True)
axs[1, 0].axvline(1, color=GRAY, ls=(0, (3, 2)), lw=0.8, zorder=0)
axs[1, 0].set(xlabel='$K/P$ ($P=8$)', ylabel='Build time (s)', title='Kernel-count sweep')
letter(axs[1, 0], 'd')

plot_lines(axs[1, 1], kp, inst_pts, 'throughput', 'throughput_ci95_low', 'throughput_ci95_high', logx=True)
axs[1, 1].axvline(1, color=GRAY, ls=(0, (3, 2)), lw=0.8, zorder=0)
axs[1, 1].set(xlabel='$K/P$ ($P=8$)', ylabel='Throughput (circuits/s)', title='Throughput saturation')
letter(axs[1, 1], 'e')

plot_lines(axs[1, 2], kp, inst_pts, 'mean_cpu_s', 'cpu_ci95_low_s', 'cpu_ci95_high_s', logx=True)
axs[1, 2].axvline(1, color=GRAY, ls=(0, (3, 2)), lw=0.8, zorder=0)
axs[1, 2].set(xlabel='$K/P$ ($P=8$)', ylabel='Aggregate CPU time (s)', title='CPU work vs. kernel count')
letter(axs[1, 2], 'f')

fig.legend(
    handles, [w[1] for w in WORKFLOWS],
    loc='lower center', ncol=3, frameon=False, bbox_to_anchor=(0.5, 0.015),
)
fig.savefig(FIG / 'fig_unified_build_scaling.pdf')
fig.savefig(FIG / 'fig_unified_build_scaling.svg')
plt.close(fig)

# --- Figure S1 ---
widths = [16, 32, 64, 128]
layers = [4, 16, 64]
fig, axs = plt.subplots(1, 2, figsize=(7.05, 3.35))
fig.subplots_adjust(left=0.08, right=0.995, top=0.84, bottom=0.22, wspace=0.32)

h_s1 = plot_lines(axs[0], widths, [(16, q, 4, 8) for q in widths], 'mean_s', 'ci95_low_s', 'ci95_high_s')
axs[0].set(xlabel='Qubits', ylabel='Build time (s)', title='Width sweep; $K=U=16$, 4 layers')
letter(axs[0], 'a', x=-0.16)
axs[0].set_xticks(widths)

plot_lines(axs[1], layers, [(16, 32, n, 8) for n in layers], 'mean_s', 'ci95_low_s', 'ci95_high_s')
axs[1].set(xlabel='Circuit layers', ylabel='Build time (s)', title='Layer sweep; $K=U=16$, 32 qubits')
letter(axs[1], 'b', x=-0.16)
axs[1].set_xticks(layers)

fig.legend(
    h_s1, [w[1] for w in WORKFLOWS],
    loc='lower center', ncol=3, frameon=False, bbox_to_anchor=(0.5, 0.02),
)
fig.savefig(FIG / 'fig_unified_build_size.pdf')
fig.savefig(FIG / 'fig_unified_build_size.svg')
plt.close(fig)
print('wrote', FIG / 'fig_unified_build_scaling.pdf', FIG / 'fig_unified_build_size.pdf')
