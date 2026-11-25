"""Versioned, SDK-independent input contract for the first direct frontend.

Copyright QCFlow 2026. Licensed under Apache-2.0.
"""
import hashlib
import json
import math
from collections.abc import Mapping

VERSION = "0.1"
GATES = {
    "id": (1, 0), "h": (1, 0), "x": (1, 0), "y": (1, 0), "z": (1, 0),
    "s": (1, 0), "sdg": (1, 0), "t": (1, 0), "tdg": (1, 0), "sx": (1, 0),
    "rx": (1, 1), "ry": (1, 1), "rz": (1, 1), "p": (1, 1), "u3": (1, 3),
    "cx": (2, 0), "cz": (2, 0), "swap": (2, 0), "cp": (2, 1),
    "measure": (1, 0), "reset": (1, 0),
}
FUNCTIONS = {"sin": math.sin, "cos": math.cos, "tan": math.tan, "exp": math.exp,
             "log": math.log, "abs": abs, "asin": math.asin, "acos": math.acos,
             "atan": math.atan}


class QLLVMError(ValueError):
    def __init__(self, code, message, path=""):
        self.code, self.path = code, path
        super().__init__(f"{code}{' at ' + path if path else ''}: {message}")

    def to_dict(self):
        return {"code": self.code, "path": self.path, "message": str(self)}


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False,
                      allow_nan=False)


def finite(value, path=""):
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise QLLVMError("INVALID_NUMBER", "expected a real number", path)
    try:
        value = float(value)
    except (OverflowError, ValueError) as error:
        raise QLLVMError("INVALID_NUMBER", "number is out of range", path) from error
    if not math.isfinite(value):
        raise QLLVMError("INVALID_NUMBER", "number must be finite", path)
    return value


def integer(value, minimum, path):
    if type(value) is not int or value < minimum:
        raise QLLVMError("INVALID_INTEGER", f"expected an integer >= {minimum}", path)
    return value


def fields(value, required, optional=(), path=""):
    if not isinstance(value, dict) or set(value) - set(required) - set(optional) or set(required) - set(value):
        raise QLLVMError("INVALID_CONTRACT", f"expected fields {sorted(required)}; optional {sorted(optional)}", path)


def constant(value):
    return {"op": "const", "value": finite(value)}


def validate_expression(expr, parameters, path="expression", depth=0):
    if depth > 64 or not isinstance(expr, dict):
        raise QLLVMError("INVALID_EXPRESSION", "invalid or excessively nested expression", path)
    op = expr.get("op")
    if not isinstance(op, str):
        raise QLLVMError("INVALID_EXPRESSION", "operation must be a string", path)
    if op == "const":
        fields(expr, ("op", "value"), path=path); finite(expr["value"], path)
    elif op == "param":
        fields(expr, ("op", "id"), path=path)
        if not isinstance(expr["id"], str) or expr["id"] not in parameters:
            raise QLLVMError("UNKNOWN_PARAMETER", str(expr.get("id")), path)
    else:
        fields(expr, ("op", "args"), path=path)
        args = expr["args"]
        if not isinstance(args, list) or op not in {"add", "mul", "pow", *FUNCTIONS}:
            raise QLLVMError("INVALID_EXPRESSION", f"unsupported expression {op}", path)
        if (op in ("add", "mul") and len(args) < 2 or
                op == "pow" and len(args) != 2 or op in FUNCTIONS and len(args) != 1):
            raise QLLVMError("INVALID_EXPRESSION", "invalid expression arity", path)
        for i, arg in enumerate(args):
            validate_expression(arg, parameters, f"{path}.args[{i}]", depth + 1)


def evaluate(expr, bindings):
    op = expr["op"]
    if op == "const":
        return finite(expr["value"])
    if op == "param":
        return bindings[expr["id"]]
    args = [evaluate(arg, bindings) for arg in expr["args"]]
    try:
        if op == "add":
            result = sum(args)
        elif op == "mul":
            result = math.prod(args)
        elif op == "pow":
            result = args[0] ** args[1]
        else:
            result = FUNCTIONS[op](*args)
        return finite(result)
    except (ValueError, OverflowError, ZeroDivisionError, TypeError) as error:
        raise QLLVMError("PARAMETER_DOMAIN", f"cannot evaluate {op}: {error}") from error


def validate_program(data):
    fields(data, ("schema_version", "kind", "name", "num_qubits", "num_clbits",
                  "parameters", "operations", "global_phase", "registers", "source"))
    if data["schema_version"] != VERSION or data["kind"] != "qllvm.program":
        raise QLLVMError("SCHEMA_VERSION", "expected qllvm.program version 0.1")
    if not isinstance(data["name"], str) or not data["name"]:
        raise QLLVMError("INVALID_NAME", "program name must be a nonempty string")
    nq = integer(data["num_qubits"], 1, "num_qubits")
    nc = integer(data["num_clbits"], 0, "num_clbits")
    if not isinstance(data["parameters"], list) or not isinstance(data["operations"], list):
        raise QLLVMError("INVALID_CONTRACT", "parameters and operations must be arrays")
    ids, names = set(), set()
    for p in data["parameters"]:
        fields(p, ("id", "name", "dtype"))
        if (not isinstance(p["id"], str) or not p["id"] or not isinstance(p["name"], str)
                or not p["name"] or p["id"] in ids or p["name"] in names or p["dtype"] != "float64"):
            raise QLLVMError("INVALID_PARAMETER", "parameter ids/names must be unique float64 scalars")
        ids.add(p["id"]); names.add(p["name"])
    validate_expression(data["global_phase"], ids, "global_phase")
    for i, op in enumerate(data["operations"]):
        path = f"operations[{i}]"
        fields(op, ("name", "qubits", "clbits", "params", "source"), path=path)
        name = op["name"]
        if not isinstance(name, str) or name not in {*GATES, "barrier"}:
            raise QLLVMError("UNSUPPORTED_OPERATION", str(name), path)
        if not isinstance(op["source"], str):
            raise QLLVMError("INVALID_CONTRACT", "source must be an operation path", path)
        for key, limit in (("qubits", nq), ("clbits", nc)):
            indices = op[key]
            if (not isinstance(indices, list) or any(type(v) is not int or not 0 <= v < limit for v in indices)
                    or len(set(indices)) != len(indices)):
                raise QLLVMError("INVALID_RESOURCE", f"invalid {key}", path)
        nqubits, nparams = GATES.get(name, (len(op["qubits"]), 0))
        if (not op["qubits"] or len(op["qubits"]) != nqubits or
                len(op["clbits"]) != (1 if name == "measure" else 0) or
                not isinstance(op["params"], list) or len(op["params"]) != nparams):
            raise QLLVMError("INVALID_ARITY", name, path)
        for j, expr in enumerate(op["params"]):
            validate_expression(expr, ids, f"{path}.params[{j}]")
    if not isinstance(data["registers"], list):
        raise QLLVMError("INVALID_CONTRACT", "registers must be an array")
    regnames = set()
    for reg in data["registers"]:
        fields(reg, ("name", "bits"))
        if (not isinstance(reg["name"], str) or not reg["name"] or reg["name"] in regnames or
                not isinstance(reg["bits"], list) or
                any(type(b) is not int or not 0 <= b < nc for b in reg["bits"]) or
                len(set(reg["bits"])) != len(reg["bits"])):
            raise QLLVMError("INVALID_REGISTER", "invalid classical register")
        regnames.add(reg["name"])
    fields(data["source"], ("frontend", "version", "capture_scope"))
    if (any(not isinstance(v, str) for v in data["source"].values()) or
            data["source"]["capture_scope"] != "circuit_only"):
        raise QLLVMError("INVALID_CAPTURE_SCOPE", "v0.1 imports circuits, not outer host workflows")


class Program:
    """Immutable snapshot; to_dict returns a copy, never a live SDK object."""
    def __init__(self, data):
        validate_program(data)
        self._json = canonical(data)

    def to_dict(self):
        return json.loads(self._json)

    @property
    def fingerprint(self):
        return hashlib.sha256(self._json.encode()).hexdigest()

    @property
    def parameters(self):
        return tuple(p["name"] for p in self.to_dict()["parameters"])

    @property
    def capture_scope(self):
        return "circuit_only"

    def bind(self, values=None):
        values = {} if values is None else values
        if not isinstance(values, Mapping) or set(values) != set(self.parameters):
            raise QLLVMError("PARAMETER_BINDINGS", f"expected exactly {self.parameters}")
        data = self.to_dict()
        bindings = {p["id"]: finite(values[p["name"]], p["name"]) for p in data["parameters"]}
        for op in data["operations"]:
            op["params"] = [constant(evaluate(expr, bindings)) for expr in op["params"]]
        data["global_phase"] = constant(evaluate(data["global_phase"], bindings))
        data["parameters"] = []
        return Program(data)

    def result_contract(self):
        data = self.to_dict()
        return {"schema_version": VERSION, "kind": "qllvm.results", "num_clbits": data["num_clbits"],
                "bit_order": "c[n-1]...c[0]", "registers": data["registers"],
                "measurements": [{"operation": i, "qubit": op["qubits"][0], "clbit": op["clbits"][0]}
                                 for i, op in enumerate(data["operations"]) if op["name"] == "measure"],
                "unwritten_bits": "zero", "repeated_writes": "last_write_wins"}


class ProgramBuilder:
    def __init__(self, num_qubits, num_clbits=0, name="main", frontend="builder", version="0.1"):
        self.data = {"schema_version": VERSION, "kind": "qllvm.program", "name": name,
                     "num_qubits": num_qubits, "num_clbits": num_clbits, "parameters": [],
                     "operations": [], "global_phase": constant(0), "registers": [],
                     "source": {"frontend": frontend, "version": version, "capture_scope": "circuit_only"}}

    def add_operations(self, operations):
        self.data["operations"].extend(operations)

    def finish(self):
        return Program(self.data)
