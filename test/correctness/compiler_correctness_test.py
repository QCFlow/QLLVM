#!/usr/bin/env python3
"""
compiler correctness test script

Test qllvm compiler with QASM programs in MQTBench, support different scales, optimization levels,
basic gate set、QPU backend and topology configuration. Only verify fidelity on 10-qubit linear topology (linear10).

Note: When using -qpu-config to specify topology, ensure qllvm correctly recognizes qasm-backend
(the backend_name in tools/driver/qllvm.in needs to correctly parse the accName with [qcor_qpu_config:...]).
"""

import argparse
import os
import sys
import subprocess
import shutil
import tempfile
from pathlib import Path


# test/correctness/ harness; shared assets live under test/
_SCRIPT_DIR = Path(__file__).resolve().parent
_TEST_ROOT = _SCRIPT_DIR.parent
os.chdir(_TEST_ROOT)
if str(_TEST_ROOT) not in sys.path:
    sys.path.insert(0, str(_TEST_ROOT))

# Add qllvm to PATH
_qllvm_bin = os.path.expanduser(os.environ.get("XACC_DIR", "~/.xacc")) + "/bin"
_qllvm_alt = os.path.expanduser("~/.qllvm/bin")
_path = os.environ.get("PATH", "")
if os.path.isdir(_qllvm_alt) and _qllvm_alt not in _path:
    _path = _qllvm_alt + ":" + _path
if _qllvm_bin not in _path:
    _path = _qllvm_bin + ":" + _path
os.environ["PATH"] = _path


# --- Scale limits (qubit count) ---
SCALE_LIMITS = {"small": 10, "medium": 20, "large": 30}

# --- Basic gate sets ---
BASICGATE_SETS = {
    "cz": "[rx,ry,rz,h,cz]",
    "cx": "[rx,ry,rz,h,cx]",
    "tyxonq": "[rz,sx,x,cx]",
    "u3": "[u3,cz]",
}
DEFAULT_BASICGATES = list(BASICGATE_SETS.keys())

# --- Topology options ---
Backend_OPTIONS = {"qasm-backend","tianyan","originquantum"}
DEFAULT_Backend = {"qasm-backend","tianyan","originquantum"}
TOPOLOGY_OPTIONS = ["none", "linear10", "linear30", "grid12x12", "eagle127","tianyan176","wukong72"]
DEFAULT_TOPOLOGIES = ["none", "linear10", "linear30", "grid12x12", "eagle127","tianyan176","wukong72"]

# --- Fidelity threshold ---
FIDELITY_THRESHOLD = 0.999

# --- Config directory ---
CONFIG_DIR = _TEST_ROOT / "qpu_configs"


def extract_qubit_count(filepath):
    """extract the qubit count from the MQTBench file name, format: xxx_nativegates_ibm_qiskit_opt0_N.qasm"""
    name = Path(filepath).stem
    parts = name.split("_")
    if len(parts) >= 1:
        try:
            return int(parts[-1])
        except ValueError:
            pass
    # Fallback: parse qreg from file
    try:
        with open(filepath) as f:
            for line in f:
                if "qreg" in line or "qubit" in line:
                    import re
                    m = re.search(r"\[(\d+)\]", line)
                    if m:
                        return int(m.group(1))
    except Exception:
        pass
    return None


def get_mqtbench_qasm_files(mqtbench_dir, scale):
    """get the list of MQTBench QASM files with the specified scale."""
    if scale not in SCALE_LIMITS:
        raise ValueError(f"Unknown scale: {scale}. Use: {list(SCALE_LIMITS.keys())}")
    max_qubits = SCALE_LIMITS[scale]

    files = []
    for p in Path(mqtbench_dir).rglob("*.qasm"):
        n = extract_qubit_count(p)
        if n is not None and n <= max_qubits:
            files.append((p, n))
    return sorted(files, key=lambda x: (x[1], str(x[0])))


def build_compile_cmd(
    qasm_path,
    run_dir,
    opt_level="O1",
    basicgate=None,
    qpu="qasm-backend",
    qpu_config=None,
    sabre_cpp=False,
    initial_mapping=None,
):
    """build the qllvm compile command. run_dir is the independent working directory for this run."""
    base = Path(qasm_path).stem
    run_dir = Path(run_dir)
    run_dir.mkdir(parents=True, exist_ok=True)
    work_qasm = run_dir / (base + ".qasm")
    shutil.copy(qasm_path, work_qasm)

    cmd = [
        "qllvm",
        str(work_qasm),
        "-qrt", "nisq",
        "-qpu", qpu,
        "-" + opt_level,
    ]
    if basicgate:
        cmd.append("-basicgate=" + BASICGATE_SETS[basicgate])
    if qpu_config:
        cmd.extend(["-qpu-config", str(qpu_config)])
    if sabre_cpp:
        cmd.append("-sabre-cpp")
    if initial_mapping is not None:
        cmd.extend(["-initial-mapping", initial_mapping])

    return cmd, run_dir, base


def run_compile(cmd, cwd):
    """execute compilation, return (success, output)"""
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd,
            capture_output=True,
            text=True,
            timeout=300,
        )
        return result.returncode == 0, result.stdout + result.stderr
    except subprocess.TimeoutExpired:
        return False, "Timeout"
    except Exception as e:
        return False, str(e)


def verify_fidelity(original_qasm, compiled_qasm,topo):
    """verify fidelity using noise-free simulator, fidelity > 0.999 is considered correct."""
    try:
        from qiskit.quantum_info.analysis import hellinger_fidelity
        from utils.Correctness_verification import fidelity
    except ImportError as e:
        return None, f"Import error: {e}"

    try:
        count1, count2 = fidelity(str(compiled_qasm), str(original_qasm),str(topo))
        fid = hellinger_fidelity(count1, count2)
        return fid, None
    except Exception as e:
        return None, str(e)


def main():
    parser = argparse.ArgumentParser(
        description="qllvm compiler correctness test",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python test/correctness/compiler_correctness_test.py
  python test/correctness/compiler_correctness_test.py --scale medium --opt O0
  python test/correctness/compiler_correctness_test.py --basicgate cz cx tyxonq
  python test/correctness/compiler_correctness_test.py --qpu-config linear10 linear30
  python test/correctness/compiler_correctness_test.py --sabre-cpp --initial-mapping "[0,1,2,3,4,5,6,7,8,9]"
        """,
    )
    parser.add_argument(
        "--scale",
        choices=list(SCALE_LIMITS.keys()),
        default="small",
        help="test scale: small(≤10), medium(≤20), large(≤30), default small",
    )
    parser.add_argument(
        "--opt",
        choices=["O0", "O1"],
        default="O1",
        help="optimization level, default O1",
    )
    parser.add_argument(
        "--basicgate",
        nargs="+",
        choices=list(BASICGATE_SETS.keys()),
        default=DEFAULT_BASICGATES,
        help="basic gate set: cz=[rx,ry,rz,h,cz], cx=[rx,ry,rz,h,cx], tyxonq=[rz,sx,x,cx], default test all",
    )
    parser.add_argument(
        "--qpu",
        nargs="+",
        choices=list(Backend_OPTIONS),
        default=list(Backend_OPTIONS),
        # default="qasm-backend",
        help="QPU backend, default Backend_OPTIONS",
    )
    parser.add_argument(
        "--qpu-config",
        nargs="+",
        choices=TOPOLOGY_OPTIONS,
        default=DEFAULT_TOPOLOGIES,
        help="topology configuration: none, linear10(only small), linear30, grid12x12, eagle127, default test none+linear10+linear30+grid12x12",
    )
    parser.add_argument(
        "--sabre-cpp",
        action="store_true",
        # default=None,
        help="use C++ SABRE mapping (requires -initial-mapping -qpu-config)",
    )
    parser.add_argument(
        "--initial-mapping",
        default=None,
        help="initial qubit mapping, like '[0,1,2,3,4]'",
    )
    parser.add_argument(
        "--mqtbench",
        default=str(_TEST_ROOT / "MQTBench"),
        help="MQTBench directory path",
    )
    parser.add_argument(
        "--max-files",
        type=int,
        default=None,
        help="maximum number of QASM files to test (for quick testing)",
    )
    parser.add_argument(
        "--no-verify",
        action="store_true",
        help="skip fidelity verification (even if the condition is met)",
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="detailed output",
    )
    args = parser.parse_args()

    # linear10 only small scale
    qpu_configs = list(args.qpu_config)
    if args.scale != "small" and "linear10" in qpu_configs:
        qpu_configs = [c for c in qpu_configs if c != "linear10"]

    mqtbench = Path(args.mqtbench)
    if not mqtbench.exists():
        print(f"error: MQTBench directory not found: {mqtbench}")
        sys.exit(1)

    # generate topology configuration file
    from utils.topology_configs import generate_topology_config, get_config_path

    config_paths = {"none": None}
    for topo in qpu_configs:
        if topo == "none":
            continue
        path, ok = generate_topology_config(CONFIG_DIR, topo)
        if ok:
            config_paths[topo] = os.path.abspath(path)
        else:
            print(f"warning: cannot generate topology {topo} (like eagle127 needs to install qiskit), skip")
            qpu_configs = [c for c in qpu_configs if c != topo]

    qasm_list = get_mqtbench_qasm_files(mqtbench, args.scale)
    if args.max_files:
        qasm_list = qasm_list[: args.max_files]
    if not qasm_list:
        print(f"no QASM files found for scale {args.scale} (≤{SCALE_LIMITS[args.scale]} qubits)")
        sys.exit(1)

    print(f"scale: {args.scale} (≤{SCALE_LIMITS[args.scale]} qubits)")
    print(f"optimization: {args.opt}, basic gates: {args.basicgate}")
    print(f"QPU: {args.qpu}, topology: {qpu_configs}")
    print(f"test case number: {len(qasm_list)}")
    print("-" * 60)
    # only verify correctness when linear10 and small
    do_verify = (
        not args.no_verify
        and args.scale == "small"
        and (("linear10" in qpu_configs) or ("tianyan176" in qpu_configs)or ("wukong72" in qpu_configs))
    )

    with tempfile.TemporaryDirectory(prefix="qllvm_test_") as work_dir:
        work_dir = Path(work_dir)
        results = {"pass": 0, "fail": 0, "skip_verify": 0}
        basegate_list = args.basicgate

        for qasm_path, nq in qasm_list:
            base_name = qasm_path.stem
            for backend_t in args.qpu:
                if backend_t == "tianyan":
                    temp_SETS = {
                        "cz": "[rx,ry,rz,h,cz]",
                    }
                    basegate_list = list(temp_SETS.keys())
                elif backend_t == "originquantum":
                    temp_SETS = {
                        "u3": "[u3,cz]",
                    }
                    basegate_list = list(temp_SETS.keys())

                for topo in qpu_configs:
                    for basicgate in basegate_list:
                        cfg_path = config_paths.get(topo)
                        # sabre-cpp needs -qpu-config
                        if args.sabre_cpp and (cfg_path is None):
                            continue

                        run_name = f"{base_name}/{basicgate}/{topo}/{backend_t}"
                        run_dir = work_dir / base_name / basicgate / topo
                        if args.verbose:
                            print(f"[Compile] {run_name}")
                        
                        cmd, cwd, base = build_compile_cmd(
                            qasm_path,
                            run_dir,
                            opt_level=args.opt,
                            basicgate=basicgate,
                            qpu=backend_t,
                            qpu_config=cfg_path,
                            sabre_cpp=args.sabre_cpp,
                            initial_mapping=args.initial_mapping,
                        )
                        ok, out = run_compile(cmd, cwd)
                        # print(cmd)

                        if backend_t == "tianyan" or backend_t == "originquantum":
                            compiled_path = run_dir / (base + "_compiled.py")
                        else:
                            compiled_path = run_dir / (base + "_compiled.qasm")
                        
                        if not ok:
                            print(f"  [Failed] {run_name}")
                            if args.verbose:
                                print(out[:500])
                            results["fail"] += 1
                            continue
                        
                        if not os.path.exists(compiled_path):
                            print(f"  [Failed] {run_name} (No compilation output generated)") 
                            results["fail"] += 1
                            continue

                        results["pass"] += 1
                        # Correctness verification: Only linear10 + small
                        if do_verify and (topo == "linear10" or topo == "tianyan176" or topo == "wukong72"):
                            fid, err = verify_fidelity(qasm_path, compiled_path,topo)
                            if err:
                                print(f"  [Verification failed] {run_name}: {err}")
                                results["fail"] += 1
                                results["pass"] -= 1
                            elif fid is not None:
                                if fid >= FIDELITY_THRESHOLD:
                                    print(f"  [Correct] {run_name} Fidelity={fid:.6f}")
                                else:
                                    print(f"  [Wrong] {run_name} Fidelity={fid:.6f} < {FIDELITY_THRESHOLD}")
                                    results["fail"] += 1
                                    results["pass"] -= 1
                            else:
                                results["skip_verify"] += 1

        print("-" * 60)
        print(f"Pass: {results['pass']}, Failure: {results['fail']}")
        if results["skip_verify"]:
            print(f"Skip verification: {results['skip_verify']}")

    sys.exit(0 if results["fail"] == 0 else 1)


if __name__ == "__main__":
    main()
