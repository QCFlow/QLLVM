"""MPI hybrid unified vs split build timing (shared by run_mpi_experiments.py)."""
from __future__ import annotations

import os
import shutil
import subprocess
import time
from pathlib import Path

from experiment_B_k_scan import (  # noqa: E402
    _apply_ld_path,
    _qasm_to_tmp,
    _resolve_qllvm_compile,
    _resolve_qllvm_home,
    _runtime_a,
    _unlink,
)


def mpi_wrapper() -> str:
    return os.environ.get("MPICXX") or shutil.which("mpicxx") or shutil.which("mpic++") or "mpicxx"


def mpi_flags(mpicxx: str, flag: str) -> list[str]:
    try:
        return subprocess.check_output([mpicxx, flag], text=True).strip().split()
    except Exception:
        return []


def mpi_cxxflags(qhome: Path) -> tuple[list[str], bool]:
    inc = qhome / "include/qllvm"
    src = os.environ.get("QLLVM_SRC", "")
    flags = ["-std=c++17"]
    ok = False
    if (inc / "qir-runner-runtime.h").is_file():
        flags.append(f"-I{inc}")
        ok = True
    if (inc / "qllvm-mpi-runtime.h").is_file():
        ok = True
    if src:
        rt = Path(src) / "mlir/qir_runner_runtime"
        mt = Path(src) / "mlir/qllvm_mpi_runtime"
        if (rt / "qir-runner-runtime.h").is_file():
            flags.extend([f"-I{rt}", f"-I{mt}"])
            ok = True
    flags.extend(mpi_flags(mpi_wrapper(), "--showme:compile"))
    return flags, ok


def mpi_runtime_a(qhome: Path) -> Path | None:
    src = os.environ.get("QLLVM_SRC", "")
    if src:
        t = Path(src) / "build/mlir/qllvm_mpi_runtime/libqllvm-mpi-runtime.a"
        if t.is_file():
            return t
    t = qhome / "lib/libqllvm-mpi-runtime.a"
    return t if t.is_file() else None


def stub_linecount_mpi(names: list[str]) -> int:
    lines = ['#include "qllvm-mpi-runtime.h"', ""]
    for n in names:
        lines.extend(
            [
                f'extern "C" void __internal_mlir_{n}(void) {{',
                f'  qllvm_mpi_execute_kernel("{n}", 0);',
                "}",
                "",
            ]
        )
    return len("\n".join(lines).splitlines())


def time_unified_mpi(
    d: Path,
    qas: list[str],
    qllvm: str,
    cuda_arch: str,
    cuda_path: str,
) -> float:
    for p in d.glob("k*.bc"):
        _unlink(p)
    _unlink(d / "app_mpi_u")
    cmd = [
        qllvm,
        "main.cpp",
        "kernel.cu",
        *qas,
        "-o",
        "app_mpi_u",
        "-qpu",
        "qir-runner",
        "-mpi",
        "-mpi-mode",
        "sim",
        "-cuda-arch",
        cuda_arch,
        "-no-bc-cache",
    ]
    if cuda_path:
        cmd += ["-cuda-path", cuda_path]
    t0 = time.perf_counter()
    subprocess.run(cmd, cwd=d, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return time.perf_counter() - t0


def time_split_mpi(
    d: Path,
    names: list[str],
    nvcc: str,
    mpicxx: str,
    cxxflags: list[str],
    qlcom: str,
    opt: str,
    qir_rt_a: Path,
    mpi_rt_a: Path,
    cuda_lib64: Path,
    cuda_arch: str,
) -> float:
    for p in d.glob("k*.bc"):
        _unlink(p)
    for p in d.glob("*.o"):
        _unlink(p)
    for name in ("app_mpi_s", "__qir_runner_mpi_kernels.cpp", "__qir_runner_mpi_kernels.o"):
        _unlink(d / name)

    link_flags = mpi_flags(mpicxx, "--showme:link")
    t0 = time.perf_counter()

    subprocess.run(
        [nvcc, "-x", "cu", "-c", "kernel.cu", "-o", "kernel_cuda.o", f"-arch={cuda_arch}"],
        cwd=d,
        check=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    subprocess.run(
        [mpicxx, "-c", *cxxflags, "-o", "main_classical.o", "main.cpp"],
        cwd=d,
        check=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    for n in names:
        tmp = d / f"__tmp_hybrid_{n}.qasm"
        _qasm_to_tmp(mpicxx, d / f"{n}.qasm", tmp)
        subprocess.run(
            [
                qlcom,
                str(tmp),
                "-qrt",
                "nisq",
                "-qpu",
                "qir-runner",
                opt,
                "-emit-backend=qir-runner",
                f"-output-path={n}.bc",
                "-basicgate=[rx,ry,rz,cz,h,x,y,z]",
            ],
            cwd=d,
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        _unlink(tmp)

    body = ['#include "qllvm-mpi-runtime.h"', ""]
    for n in names:
        body.extend(
            [
                f'extern "C" void __internal_mlir_{n}(void) {{',
                f'  qllvm_mpi_execute_kernel("{n}", 0);',
                "}",
                "",
            ]
        )
    sp = d / "__qir_runner_mpi_kernels.cpp"
    sp.write_text("\n".join(body))
    subprocess.run(
        [mpicxx, "-c", *cxxflags, "-o", "__qir_runner_mpi_kernels.o", str(sp)],
        cwd=d,
        check=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    cuda_lib = str(cuda_lib64)
    if not Path(cuda_lib).is_dir():
        cuda_lib = str(cuda_lib64.parent / "lib")

    subprocess.run(
        [
            mpicxx,
            "-o",
            "app_mpi_s",
            "main_classical.o",
            "kernel_cuda.o",
            "__qir_runner_mpi_kernels.o",
            str(mpi_rt_a),
            str(qir_rt_a),
            f"-L{cuda_lib}",
            "-lcudart",
            "-lcuda",
            "-ldl",
            "-lrt",
            "-lpthread",
            *link_flags,
        ],
        cwd=d,
        check=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    for f in (
        "kernel_cuda.o",
        "main_classical.o",
        "__qir_runner_mpi_kernels.o",
        "__qir_runner_mpi_kernels.cpp",
    ):
        _unlink(d / f)
    return time.perf_counter() - t0


def prepare_mpi_env(repo: Path) -> dict:
    qllvm = shutil.which("qllvm")
    if not qllvm:
        raise RuntimeError("qllvm not in PATH")
    qhome = _resolve_qllvm_home(qllvm)
    qlcom, extra_ld = _resolve_qllvm_compile(qhome)
    if not qlcom or not Path(qlcom).is_file():
        raise RuntimeError("qllvm-compile not found")
    _apply_ld_path(qhome, extra_ld)

    from experiment_B_k_scan import _cuda_path, _cxxflags  # noqa: WPS433

    cuda_path = _cuda_path()
    if not cuda_path:
        raise RuntimeError("CUDA_PATH / nvcc required")
    nvcc = os.environ.get("NVCC") or shutil.which("nvcc")
    mpicxx = mpi_wrapper()
    if not nvcc:
        raise RuntimeError("nvcc required")
    cuda_bin = Path(cuda_path) / "bin"
    if (cuda_bin / "nvcc").is_file():
        nvcc = str(cuda_bin / "nvcc")
    os.environ["PATH"] = str(Path(nvcc).parent) + os.pathsep + os.environ.get("PATH", "")

    cxxflags, ok = mpi_cxxflags(qhome)
    if not ok:
        raise RuntimeError("MPI runtime headers missing")
    qir_rt = _runtime_a(qhome)
    mpi_rt = mpi_runtime_a(qhome)
    if not qir_rt.is_file() or not mpi_rt or not mpi_rt.is_file():
        raise RuntimeError("libqir-runner-runtime.a or libqllvm-mpi-runtime.a missing")

    return {
        "qllvm": qllvm,
        "qlcom": qlcom,
        "cuda_path": cuda_path,
        "nvcc": nvcc,
        "mpicxx": mpicxx,
        "cxxflags": cxxflags,
        "qir_rt_a": qir_rt,
        "mpi_rt_a": mpi_rt,
        "cuda_arch": os.environ.get("CUDA_ARCH", "sm_75"),
        "cuda_lib64": Path(cuda_path) / "lib64",
        "opt": os.environ.get("OPT", "-O0"),
        "reps": int(os.environ.get("MPI_SCAL_REPS", "2")),
        "kernel_cu": repo / "examples/unified_vs_split_cudaq/kernel.cu",
        "bell": repo / "examples/unified_vs_split_cudaq/bell.qasm",
    }
