#!/usr/bin/env python3
"""Create an isolated SDK environment, or explicitly refresh PyPI version pins."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import subprocess
import sys
import urllib.request

ROOT = Path(__file__).resolve().parent
PACKAGES = ["qiskit", "pytket", "pyqpanda", "pyqpanda3", "pennylane", "mqt.bench"]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--refresh-versions", action="store_true", help="Explicit online maintainer step; normal benchmark runs never access the network")
    p.add_argument("--environment", type=Path)
    p.add_argument("--native", action="store_true", help="Install official pytest/Benchpress dependencies too")
    p.add_argument("--metrics-environment", type=Path, help="Separate environment for unmodified upstream QMetric (Python 3.10/3.11)")
    args = p.parse_args()
    if args.refresh_versions:
        data = {"checked_at": datetime.now(timezone.utc).isoformat(), "packages": {}}
        for package in PACKAGES:
            url = f"https://pypi.org/pypi/{package}/json"
            req = urllib.request.Request(url, headers={"User-Agent": "QLLVM-benchmarks"})
            with urllib.request.urlopen(req, timeout=60) as response:
                result = json.load(response)
            version = result["info"]["version"]
            data["packages"][package] = {"version": version, "metadata_url": url,
                "requires_python": result["info"]["requires_python"],
                "uploaded_at": result["releases"][version][0]["upload_time_iso_8601"]}
            print(package, version, flush=True)
        (ROOT / "versions.json").write_text(json.dumps(data, indent=2) + "\n")
    if args.environment:
        if sys.version_info < (3, 11):
            p.error("Use Python 3.11+ for current PennyLane and MQT Bench")
        args.environment = args.environment.resolve()
        python = args.environment / "bin/python"
        if not python.exists():
            subprocess.run([sys.executable, "-m", "venv", str(args.environment)], check=True)
        versions = json.loads((ROOT / "versions.json").read_text())["packages"]
        pins = [f"{name}=={versions[name]['version']}" for name in PACKAGES]
        if args.native:
            pins += ["pytest", "pytest-benchmark", "pytest-timeout", "pytest-json-report",
                     "qiskit-ibm-runtime", "pytket-qiskit", "wrapt_timeout_decorator"]
        subprocess.run([str(python), "-m", "pip", "install", *pins], check=True)
        # Keep the run environment's exact transitive lock next to the environment.
        frozen = subprocess.check_output([str(python), "-m", "pip", "freeze"], text=True)
        (args.environment / "requirements.lock.txt").write_text(frozen)
    if args.metrics_environment:
        environment = args.metrics_environment.absolute()
        if args.environment and environment == args.environment:
            p.error("Metric and comparison environments must be different")
        python = environment / "bin/python"
        if not python.exists():
            subprocess.run([sys.executable, "-m", "venv", str(environment)], check=True)
        subprocess.run([str(python), "-m", "pip", "install", "qiskit-terra==0.24.2",
                        "numpy==1.26.4", "matplotlib==3.7.5", "networkx==3.2.1"], check=True)
        (environment / "requirements.lock.txt").write_text(subprocess.check_output([str(python), "-m", "pip", "freeze"], text=True))
    if not args.refresh_versions and not args.environment and not args.metrics_environment:
        p.error("Choose --refresh-versions, --environment or --metrics-environment")


if __name__ == "__main__":
    main()
