#!/usr/bin/env python3
"""Index every official QASM input and expand every MQT benchmark generator."""
import argparse
import ast
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tarfile

from sources import ROOT, REPOSITORIES, verify, sha


def qubit_hint(path):
    # Only a scheduling hint. Actual widths are checked by the SDK parser.
    with path.open(errors="replace") as stream:
        header = stream.read(1024 * 1024)
    return qubit_hint_text(header)


def qubit_hint_text(header):
    header = re.sub(r"//[^\n]*", "", header)
    total = sum(int(n) for n in re.findall(r"\bqreg\s+\w+\s*\[\s*(\d+)\s*\]", header))
    total += sum(int(n) for n in re.findall(r"\bqubit\s*\[\s*(\d+)\s*\]", header))
    return total or None


def identifier(suite, relative):
    # Paths, not basenames, distinguish circuits in different suite categories.
    slug = re.sub(r"[^a-zA-Z0-9_-]+", "-", str(relative)).strip("-")[:120]
    return suite + "-" + slug + "-" + hashlib.sha256(str(relative).encode()).hexdigest()[:10]


def official_mqt_sizes():
    path = ROOT / "upstream/mqtbench/tests/test_bench.py"
    for node in ast.parse(path.read_text()).body:
        if isinstance(node, (ast.Assign, ast.AnnAssign)):
            target = node.targets[0] if isinstance(node, ast.Assign) else node.target
            if isinstance(target, ast.Name) and target.id == "SPECIAL_QUBIT_COUNTS":
                return ast.literal_eval(node.value)
    # The fallback is explicit and all invalid parameters remain in the catalog.
    return {}


def generate_one(request):
    # Load the exact vendored generator code, not a reimplementation.
    sys.path.insert(0, str(ROOT / "upstream/mqtbench/src"))
    from mqt.bench import BenchmarkLevel, get_benchmark
    from mqt.bench.targets.devices import get_device
    from mqt.bench.targets.gatesets import get_target_for_gateset
    from qiskit import qasm2, qasm3, qpy, transpile
    level = request["level"]
    target = None
    if level == "nativegates":
        target = get_target_for_gateset(request["gateset"], request["size"])
    elif level == "mapped":
        target = get_device(request["device"])
    circuit = get_benchmark(request["benchmark"], BenchmarkLevel[level.upper()],
                            request["size"], opt_level=0, target=target)
    folder = Path(request["directory"])
    with (folder / "original.qpy").open("wb") as handle:
        qpy.dump(circuit, handle)
    # Export a plain static circuit at opt0. Save original QPY as evidence of the
    # official generator output. Dynamic circuits stay QASM3 without stripping.
    try:
        exported = transpile(circuit, basis_gates=["u3", "cx"], optimization_level=0,
                             seed_transpiler=12345)
        path = folder / "input.qasm"
        path.write_text(qasm2.dumps(exported) + "\n")
        export = "Official generator output; u3/cx serialization at opt0, no routing; original.qpy retained"
    except Exception:
        path = folder / "input.qasm3"
        path.write_text(qasm3.dumps(circuit))
        export = "Official generator output exported as QASM3, including dynamic operations"
    result = {"file": str(path.relative_to(ROOT)), "sha256": sha(path),
              "qubits": circuit.num_qubits, "generation_status": "ready",
              "transformation": export}
    (folder / "generation.json").write_text(json.dumps(result, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "catalog.json")
    parser.add_argument("--suite", nargs="+", choices=list(REPOSITORIES), default=list(REPOSITORIES))
    parser.add_argument("--reuse-mqt", type=Path, help="Reuse an earlier MQT generation catalog from the identical source revision")
    parser.add_argument("--sizes", type=int, nargs="*", default=[], help="Additional sizes for every MQT family; official test sizes always included")
    parser.add_argument("--levels", nargs="+", choices=["alg", "indep", "nativegates", "mapped"],
                        default=["alg", "indep", "nativegates", "mapped"])
    parser.add_argument("--gateset", default="ibm_falcon")
    parser.add_argument("--device", default="ibm_falcon_27")
    parser.add_argument("--generation-timeout", type=float, default=120)
    parser.add_argument("--generate-one", type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.generate_one:
        generate_one(json.loads(args.generate_one.read_text()))
        return
    if args.output.exists():
        parser.error("Catalog already exists; use a fresh --output to preserve provenance")
    snapshots = {name: verify(name) for name in args.suite}
    rows = []
    for suite in args.suite:
        source = ROOT / "upstream" / suite
        for relative, info in snapshots[suite]["files"].items():
            path = source / relative
            if ".qasm" in path.name.lower() and path.name.endswith((".tar.gz", ".tgz")):
                with tarfile.open(path) as archive:
                    for member in archive.getmembers():
                        if not member.isfile() or Path(member.name).suffix.lower() not in (".qasm", ".qasm3"):
                            continue
                        with archive.extractfile(member) as stream:
                            width = qubit_hint_text(stream.read(1024*1024).decode(errors="replace"))
                        rows.append({"id": identifier(suite, relative + ":" + member.name),
                            "suite": suite, "family": str(Path(relative).parent), "level": "official-archive",
                            "file": str(path.relative_to(ROOT)), "sha256": info["sha256"],
                            "archive_member": member.name, "qubits": width,
                            "bytes": member.size, "generation_status": "ready",
                            "transformation": "Original official archived QASM; extracted lazily without editing"})
                continue
            if path.suffix.lower() not in (".qasm", ".qasm3"):
                continue
            rows.append({"id": identifier(suite, relative), "suite": suite,
                         "family": str(Path(relative).parent), "level": "official-file",
                         "file": str(path.relative_to(ROOT)), "sha256": info["sha256"],
                         "qubits": qubit_hint(path), "bytes": info["size"],
                         "generation_status": "ready", "transformation": "Unmodified official file"})
    # Discover the entire upstream generator catalog without a manually selected
    # algorithm list. A finite parameter grid cannot exhaust an infinite generator.
    minimums, names = {}, []
    if "mqtbench" in snapshots:
        sys.path.insert(0, str(ROOT / "upstream/mqtbench/src"))
        from mqt.bench.benchmarks import get_available_benchmark_names
        minimums = official_mqt_sizes()
        names = get_available_benchmark_names()
    reused = None
    if args.reuse_mqt:
        reused = json.loads(args.reuse_mqt.read_text())
        if reused["sources"]["mqtbench"]["revision"] != snapshots["mqtbench"]["revision"]:
            parser.error("Reused MQT generator source revision differs")
        if set(reused["mqt_generation"]["families"]) != set(names):
            parser.error("Reused MQT family catalog differs")
        for row in reused["cases"]:
            if row["suite"] == "mqtbench" and row["level"] != "official-file":
                if row.get("file") and sha(ROOT / row["file"]) != row["sha256"]:
                    parser.error("Reused MQT output checksum mismatch")
                rows.append(row)
    generation_id = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    for name in ([] if reused else names):
        for size in sorted({minimums.get(name, 3), *args.sizes}):
            for level in args.levels:
                key = f"mqtbench-{name}-{size}-{level}"
                folder = ROOT / "generated" / generation_id / key
                folder.mkdir(parents=True, exist_ok=False)
                request = {"benchmark": name, "size": size, "level": level,
                           "gateset": args.gateset, "device": args.device, "directory": str(folder)}
                request_path = folder / "request.json"
                request_path.write_text(json.dumps(request, indent=2) + "\n")
                row = {"id": key, "suite": "mqtbench", "family": name, "level": level,
                       "parameters": {k: v for k, v in request.items() if k != "directory"}}
                try:
                    with (folder / "generation.log").open("w") as log:
                        done = subprocess.run([sys.executable, "-B", str(Path(__file__).resolve()),
                                               "--generate-one", str(request_path)], stdout=log,
                                              stderr=subprocess.STDOUT, timeout=args.generation_timeout)
                    if done.returncode:
                        raise ValueError("Generator/export failed: " + str(folder / "generation.log"))
                    row.update(json.loads((folder / "generation.json").read_text()))
                except (subprocess.TimeoutExpired, ValueError) as exc:
                    row.update(generation_status="generation_failed", error=str(exc))
                rows.append(row)
                print(key, row["generation_status"], flush=True)
    catalog = {"schema_version": 1, "created_at": datetime.now(timezone.utc).isoformat(),
               "sources": {k: {x: v[x] for x in ["repository", "reference", "revision", "archive_sha256", "file_count", "bytes"]}
                           for k, v in snapshots.items()},
               "mqt_generation": {"families": names, "official_test_sizes": minimums,
                                  "extra_sizes": args.sizes, "levels": args.levels,
                                  "gateset": args.gateset, "device": args.device}, "cases": rows}
    if reused:
        catalog["mqt_generation"] = reused["mqt_generation"]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(catalog, indent=2) + "\n")
    print(f"Indexed {len(rows)} cases in {args.output}")


if __name__ == "__main__":
    main()
