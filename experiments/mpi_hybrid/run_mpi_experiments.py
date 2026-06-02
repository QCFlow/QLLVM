#!/usr/bin/env python3
"""
Exp-1: MPI hybrid build scalability — unified `qllvm -mpi` vs manual MPI split pipeline.

Outputs: results/mpi_build_scalability.csv
"""
from __future__ import annotations

import csv
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "scripts"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from experiment_B_k_scan import mean  # noqa: E402
from mpi_build_utils import (  # noqa: E402
    prepare_mpi_env,
    stub_linecount_mpi,
    time_split_mpi,
    time_unified_mpi,
)

OUT = Path(__file__).resolve().parent / "results"
WORK = OUT / "_work_build"


def _main_cpp_mpi(k: int) -> str:
    decls = "\n".join(f"void __internal_mlir_k{i:03d}(void);" for i in range(1, k + 1))
    calls = "\n".join(f"    __internal_mlir_k{i:03d}();" for i in range(1, k + 1))
    return f"""#include <mpi.h>
#include <cstdint>
#include <iostream>
extern void launchCudaKernel(int n);
extern "C" {{
void __quantum__rt__initialize(int argc, int8_t** argv);
void __quantum__rt__finalize();
{decls}
}}
int main(int argc, char** argv) {{
  MPI_Init(&argc, &argv);
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  __quantum__rt__initialize(argc, reinterpret_cast<int8_t**>(argv));
  if (rank == 0) launchCudaKernel(1 << 18);
  MPI_Barrier(MPI_COMM_WORLD);
{calls}
  __quantum__rt__finalize();
  MPI_Finalize();
  return 0;
}}
"""


def _time_mpi_run(exe: Path, np: int = 2, shots: int = 64, reps: int = 2) -> float:
    def once() -> float:
        t0 = time.perf_counter()
        subprocess.run(
            ["mpirun", "-np", str(np), str(exe), "-shots", str(shots)],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        return time.perf_counter() - t0

    return mean(reps, once)


def exp1_kernel_count(env: dict, k_values: list[int]) -> list[dict]:
    rows: list[dict] = []
    for k in k_values:
        d = WORK / f"k_{k:03d}"
        if d.exists():
            shutil.rmtree(d)
        d.mkdir(parents=True)
        shutil.copy2(env["kernel_cu"], d / "kernel.cu")
        (d / "main.cpp").write_text(_main_cpp_mpi(k))
        names = [f"k{i:03d}" for i in range(1, k + 1)]
        qas = []
        for name in names:
            shutil.copy2(env["bell"], d / f"{name}.qasm")
            qas.append(f"{name}.qasm")

        t_u = mean(
            env["reps"],
            lambda: time_unified_mpi(d, qas, env["qllvm"], env["cuda_arch"], env["cuda_path"]),
        )
        t_s = mean(
            env["reps"],
            lambda: time_split_mpi(
                d,
                names,
                env["nvcc"],
                env["mpicxx"],
                env["cxxflags"],
                env["qlcom"],
                env["opt"],
                env["qir_rt_a"],
                env["mpi_rt_a"],
                env["cuda_lib64"],
                env["cuda_arch"],
            ),
        )
        time_unified_mpi(d, qas, env["qllvm"], env["cuda_arch"], env["cuda_path"])
        np_run = min(2, k) if k >= 1 else 1
        try:
            t_run = _time_mpi_run(d / "app_mpi_u", np=np_run, reps=max(1, env["reps"] - 1))
            t_run_s = f"{t_run:.4f}"
        except Exception as e:
            print(f"[K={k}] runtime skip: {e}", file=sys.stderr)
            t_run_s = ""

        l_stub = stub_linecount_mpi(names)
        rows.append(
            {
                "experiment": "mpi_kernel_count",
                "K": k,
                "T_build_unified_mpi_s": f"{t_u:.4f}",
                "T_build_split_mpi_s": f"{t_s:.4f}",
                "build_ratio_split_over_uni": f"{t_s / t_u:.4f}" if t_u > 0 else "",
                "T_runtime_mpi_s": t_run_s,
                "N_cmd_unified": 1,
                "N_cmd_split": 6 + k,
                "L_glue_split": l_stub,
            }
        )
        print(
            f"[MPI K={k}] T_u={t_u:.3f}s T_s={t_s:.3f}s ratio={t_s/t_u:.2f}x glue={l_stub}L",
            file=sys.stderr,
        )
    return rows


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    WORK.mkdir(parents=True, exist_ok=True)
    env = prepare_mpi_env(REPO)
    k_vals = [int(x) for x in os.environ.get("MPI_K_VALUES", "1,2,4,8,16").split(",")]
    rows = exp1_kernel_count(env, k_vals)
    out = OUT / "mpi_build_scalability.csv"
    with out.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"Wrote {out}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
