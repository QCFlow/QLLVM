"""Offline executable bundles and persistent, per-job local execution.

Copyright QCFlow 2026. Licensed under Apache-2.0.
"""
from concurrent.futures import ThreadPoolExecutor
from contextlib import contextmanager
from dataclasses import dataclass
import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import threading
import time
import uuid

from .program import Program, QLLVMError, VERSION, canonical, integer


def digest(path):
    hasher = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            hasher.update(block)
    return hasher.hexdigest()


def atomic_json(path, data):
    path = Path(path)
    fd, temporary = tempfile.mkstemp(prefix=".write-", dir=path.parent)
    try:
        with os.fdopen(fd, "w") as stream:
            stream.write(canonical(data) + "\n")
            stream.flush(); os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


@contextmanager
def file_lock(path):
    with Path(path).open("a") as stream:
        fcntl.flock(stream, fcntl.LOCK_EX)
        try:
            yield
        finally:
            fcntl.flock(stream, fcntl.LOCK_UN)


@dataclass(frozen=True)
class TargetCapabilities:
    provider: str = "local"
    target: str = "basic-simulator"
    max_qubits: int = 20
    max_shots: int = 1_000_000

    def to_dict(self):
        return {"schema_version": VERSION, "kind": "qllvm.target", "provider": self.provider,
                "target": self.target, "simulator": True, "max_qubits": self.max_qubits,
                "max_shots": self.max_shots, "program_format": "openqasm2",
                "parameter_binding": "specialize_before_codegen", "classical_control": False,
                "mid_circuit_measurement": True, "reset": True, "result_kind": "counts"}


def _compile(program, compiler, output, opt_level):
    if program.parameters:
        raise QLLVMError("UNBOUND_PARAMETERS", "bind parameters before native code generation")
    with tempfile.TemporaryDirectory(prefix="qllvm-input-") as temporary:
        source = Path(temporary) / "program.json"
        source.write_text(canonical(program.to_dict()))
        command = [str(compiler), str(source), "-input-backend=program-json", "-no-entrypoint",
                   "-internal-func-name=direct", f"-O{opt_level}", "-exact-rotations",
                   "-qpu=qasm-backend", "-emit-backend=qasm-backend", f"-output-path={output}"]
        try:
            completed = subprocess.run(command, text=True, capture_output=True, timeout=120)
        except subprocess.TimeoutExpired as error:
            raise QLLVMError("COMPILER_TIMEOUT", "native compilation exceeded 120 seconds") from error
        if completed.returncode or not Path(output).is_file():
            raise QLLVMError("COMPILATION_FAILED", (completed.stderr + completed.stdout)[-8000:])


class Executable:
    def __init__(self, path):
        self.path = Path(path).resolve()
        self.manifest = json.loads((self.path / "manifest.json").read_text())
        if (self.manifest.get("schema_version") != VERSION or
                self.manifest.get("kind") != "qllvm.executable"):
            raise QLLVMError("SCHEMA_VERSION", "unsupported executable bundle")
        files = self.manifest.get("files", {})
        if set(files) != {"program.json", "parameters.json", "results.json", "target.json"}:
            raise QLLVMError("BUNDLE_CONTENTS", "unexpected or missing bundle files")
        for relative, expected in files.items():
            p = self.path / relative
            if p.is_symlink() or not p.is_file() or digest(p) != expected:
                raise QLLVMError("BUNDLE_INTEGRITY", relative)
        self.program = Program(json.loads((self.path / "program.json").read_text()))
        self.result_contract = json.loads((self.path / "results.json").read_text())
        self.target = json.loads((self.path / "target.json").read_text())
        self.parameters = json.loads((self.path / "parameters.json").read_text())
        expected_parameters = {"schema_version": VERSION, "kind": "qllvm.parameters",
                               "binding_strategy": "specialize_before_codegen",
                               "entries": self.program.to_dict()["parameters"]}
        if self.parameters != expected_parameters:
            raise QLLVMError("PARAMETER_CONTRACT", "bundle parameter ABI differs from program")
        if self.result_contract != self.program.result_contract():
            raise QLLVMError("RESULT_CONTRACT", "bundle result contract differs from program")
        expected = self.manifest.get("executable_id")
        body = {k: v for k, v in self.manifest.items() if k != "executable_id"}
        if expected != hashlib.sha256(canonical(body).encode()).hexdigest():
            raise QLLVMError("BUNDLE_INTEGRITY", "manifest fingerprint mismatch")

    @property
    def fingerprint(self):
        return self.manifest["executable_id"]

    def verify_compiler(self):
        compiler = Path(self.manifest["compiler"]["path"])
        if not compiler.is_file() or digest(compiler) != self.manifest["compiler"]["sha256"]:
            raise QLLVMError("COMPILER_CHANGED", "rebuild the bundle with the current compiler")
        return compiler


def build(program, output, compiler=None, opt_level=0, target=None):
    """Build a parameter template. Binding triggers native specialization at run time.

    A v0.1 bundle deliberately records this strategy, rather than claiming QPU
    parameter binding or a native compiled host workflow.
    """
    if not isinstance(program, Program):
        raise QLLVMError("PROGRAM_TYPE", "expected an imported Program")
    if type(opt_level) is not int or opt_level not in (0, 1):
        raise QLLVMError("OPT_LEVEL", "expected 0 or 1")
    target = target or TargetCapabilities()
    if not isinstance(target, TargetCapabilities) or target.provider not in ("local", "mock"):
        raise QLLVMError("TARGET", "v0.1 supports local or explicit mock targets")
    data = program.to_dict()
    if data["num_qubits"] > target.max_qubits:
        raise QLLVMError("TARGET_CAPACITY", "program exceeds target qubit capacity")
    if not any(op["name"] == "measure" for op in data["operations"]):
        raise QLLVMError("NO_MEASUREMENTS", "counts execution requires explicit measurements")
    compiler = compiler or os.environ.get("QLLVM_COMPILER") or shutil.which("qllvm-compile")
    if not compiler or not Path(compiler).is_file():
        raise QLLVMError("COMPILER_NOT_FOUND", "set QLLVM_COMPILER to the rebuilt qllvm-compile")
    compiler = Path(compiler).resolve()
    output = Path(output).absolute()
    output.parent.mkdir(parents=True, exist_ok=True)
    if output.exists():
        raise QLLVMError("OUTPUT_EXISTS", str(output))
    with tempfile.TemporaryDirectory(prefix=".qexe-", dir=output.parent) as temporary:
        root = Path(temporary)
        parameters = {"schema_version": VERSION, "kind": "qllvm.parameters",
                      "binding_strategy": "specialize_before_codegen", "entries": data["parameters"]}
        for name, value in (("program.json", data), ("parameters.json", parameters), ("results.json", program.result_contract()),
                            ("target.json", target.to_dict())):
            atomic_json(root / name, value)
        manifest = {"schema_version": VERSION, "kind": "qllvm.executable",
                    "capture_scope": "circuit_only", "frontend_path": "sdk-object-to-mlir",
                    "parameter_binding": "specialize_before_codegen", "opt_level": opt_level,
                    "compiler": {"path": str(compiler), "sha256": digest(compiler)},
                    "files": {p.name: digest(p) for p in root.iterdir()}}
        manifest["executable_id"] = hashlib.sha256(canonical(manifest).encode()).hexdigest()
        atomic_json(root / "manifest.json", manifest)
        # mkdir establishes ownership atomically; never overwrite another output.
        output.mkdir()
        try:
            for path in root.iterdir():
                shutil.copyfile(path, output / path.name)
        except BaseException:
            shutil.rmtree(output)
            raise
    return Executable(output)


@dataclass(frozen=True)
class Result:
    job_id: str
    counts: dict
    shots: int
    provider: str
    simulated: bool
    contract: dict

    def register_counts(self, name):
        reg = next((reg for reg in self.contract["registers"] if reg["name"] == name), None)
        if reg is None:
            raise QLLVMError("UNKNOWN_REGISTER", name)
        counts = {}
        for bits, count in self.counts.items():
            key = "".join(bits[-1-i] for i in reversed(reg["bits"]))
            counts[key] = counts.get(key, 0) + count
        return counts


class Job:
    def __init__(self, provider, job_id):
        self.provider, self.job_id = provider, job_id

    def status(self):
        return self.provider.query(self.job_id)["status"]

    def cancel(self):
        return self.provider.cancel(self.job_id)

    def result(self, timeout=None):
        start = time.monotonic()
        while True:
            data = self.provider.query(self.job_id)
            if data["status"] == "SUCCEEDED":
                raw = data["result"]
                return Result(self.job_id, dict(raw["counts"]), data["shots"],
                              data["provider"], True, data["contract"])
            if data["status"] in ("FAILED", "CANCELLED", "SUBMIT_UNKNOWN"):
                raise QLLVMError("JOB_" + data["status"], data.get("error", "job did not complete"))
            if timeout is not None and time.monotonic() - start >= timeout:
                raise TimeoutError("job is still running; timeout does not cancel it")
            time.sleep(0.01)


class LocalProvider:
    """Execute the native compiler's QASM artifact with Qiskit's BasicSimulator.

    Job files are durable; an interrupted in-process job becomes SUBMIT_UNKNOWN
    on resume and is never automatically resubmitted.
    """
    name = "local"

    def __init__(self, state_dir, workers=2):
        self.state_dir = Path(state_dir).absolute()
        self.state_dir.mkdir(parents=True, exist_ok=True)
        self._executor = ThreadPoolExecutor(max_workers=workers, thread_name_prefix="qllvm")
        self._futures = {}
        self._lock = threading.RLock()
        self._closed = False

    def describe_target(self):
        return TargetCapabilities(provider=self.name).to_dict()

    def _path(self, job_id):
        if not isinstance(job_id, str) or not re.fullmatch(r"[A-Za-z0-9_-]{1,128}", job_id):
            raise QLLVMError("JOB_ID", "invalid job id")
        return self.state_dir / (job_id + ".json")

    def submit(self, executable, bindings=None, shots=1024, seed=None, request_id=None):
        if not isinstance(executable, Executable):
            raise QLLVMError("EXECUTABLE_TYPE", "expected an Executable")
        # Reopen to verify no files changed after the handle was created.
        executable = Executable(executable.path)
        integer(shots, 1, "shots")
        if seed is not None:
            integer(seed, 0, "seed")
        bound = executable.program.bind(bindings)
        caps = self.describe_target()
        if executable.target != caps:
            raise QLLVMError("TARGET_MISMATCH", "executable was built for another target")
        if shots > caps["max_shots"] or bound.to_dict()["num_qubits"] > caps["max_qubits"]:
            raise QLLVMError("TARGET_CAPACITY", "shots or qubit limit exceeded")
        compiler = executable.verify_compiler()
        job_id = request_id if request_id is not None else uuid.uuid4().hex
        path = self._path(job_id)
        request = {"executable_id": executable.fingerprint, "program_id": bound.fingerprint,
                   "shots": shots, "seed": seed, "provider": self.name}
        with self._lock, file_lock(self.state_dir / ".submit.lock"):
            if self._closed:
                raise QLLVMError("PROVIDER_CLOSED", "provider is closed")
            if path.exists():
                previous = json.loads(path.read_text())
                if previous["request"] != request:
                    raise QLLVMError("REQUEST_CONFLICT", "request id is already bound to different inputs")
                return Job(self, job_id)
            data = {"schema_version": VERSION, "kind": "qllvm.job", "job_id": job_id,
                    "status": "QUEUED", "request": request, "shots": shots, "seed": seed,
                    "provider": self.name, "contract": executable.result_contract,
                    "owner_pid": os.getpid(), "created_at": time.time()}
            atomic_json(path, data)
            self._futures[job_id] = self._executor.submit(self._execute, job_id, bound, compiler,
                                                         executable.manifest["opt_level"])
        return Job(self, job_id)

    def _execute(self, job_id, bound, compiler, opt_level):
        path = self._path(job_id)
        with self._lock:
            data = json.loads(path.read_text())
            if data["status"] == "CANCELLED":
                return
            data["status"] = "RUNNING"; atomic_json(path, data)
        try:
            cache = self.state_dir / "compiled"
            cache.mkdir(exist_ok=True)
            key = hashlib.sha256(canonical({"program": bound.fingerprint, "compiler": digest(compiler),
                                           "opt_level": opt_level}).encode()).hexdigest()
            qasm = cache / (key + ".qasm")
            with file_lock(cache / (key + ".lock")):
                meta = cache / (key + ".json")
                if not qasm.is_file() or not meta.is_file() or digest(qasm) != json.loads(meta.read_text())["sha256"]:
                    with tempfile.TemporaryDirectory(dir=cache) as temp:
                        output = Path(temp) / "kernel.qasm"
                        _compile(bound, compiler, output, opt_level)
                        os.replace(output, qasm)
                    atomic_json(meta, {"sha256": digest(qasm)})
            data["artifact"] = {"path": str(qasm), "sha256": digest(qasm)}
            raw = self._run_artifact(qasm, data, bound)
            data["status"] = "SUCCEEDED"; data["result"] = raw
        except Exception as error:
            data["status"] = "FAILED"
            data["error"] = str(error)
            data["error_code"] = getattr(error, "code", type(error).__name__)
        with self._lock:
            data["completed_at"] = time.time(); atomic_json(path, data)

    def _run_artifact(self, qasm, data, bound):
        from qiskit import QuantumCircuit
        from qiskit.providers.basic_provider import BasicSimulator
        circuit = QuantumCircuit.from_qasm_str(qasm.read_text())
        nc = bound.to_dict()["num_clbits"]
        if circuit.num_clbits != nc:
            raise QLLVMError("RESULT_LAYOUT", "native output changed classical result width")
        raw = BasicSimulator().run(circuit, shots=data["shots"], seed_simulator=data["seed"]).result().get_counts()
        counts = {bits.replace(" ", ""): int(count) for bits, count in raw.items()}
        if sum(counts.values()) != data["shots"] or any(len(bits) != nc for bits in counts):
            raise QLLVMError("RESULT_SHAPE", "simulator result violates result contract")
        return {"counts": counts, "simulated": True, "engine": "qiskit.BasicSimulator"}

    def query(self, job_id):
        path = self._path(job_id)
        if not path.is_file():
            raise QLLVMError("JOB_NOT_FOUND", job_id)
        return json.loads(path.read_text())

    def resume(self, job_id):
        data = self.query(job_id)
        with self._lock:
            if data["status"] in ("QUEUED", "RUNNING") and job_id not in self._futures:
                try:
                    os.kill(data["owner_pid"], 0)
                except ProcessLookupError:
                    data["status"] = "SUBMIT_UNKNOWN"
                    data["error"] = "local worker exited; execution was not automatically resubmitted"
                    atomic_json(self._path(job_id), data)
        return Job(self, job_id)

    def cancel(self, job_id):
        with self._lock:
            data = self.query(job_id)
            future = self._futures.get(job_id)
            if data["status"] == "QUEUED" and future is not None and future.cancel():
                data["status"] = "CANCELLED"; atomic_json(self._path(job_id), data)
                return "CANCELLED"
            return data["status"]

    def close(self):
        with self._lock:
            self._closed = True
        self._executor.shutdown(wait=True)

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()


class MockProvider(LocalProvider):
    """Protocol fixture with explicitly synthetic results; never a hardware result."""
    name = "mock"

    def _run_artifact(self, qasm, data, bound):
        return {"counts": {"0" * bound.to_dict()["num_clbits"]: data["shots"]},
                "simulated": True, "engine": "mock-synthetic-zero"}
