#!/usr/bin/env python3
"""Generate the checked-in v0.1 contracts; no SDK imports required."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "python_qllvm/qllvm_core/schemas"


def obj(properties, required=None):
    return {"type": "object", "properties": properties,
            "required": list(properties) if required is None else required, "additionalProperties": False}


def array(item):
    return {"type": "array", "items": item}


def versioned(kind, properties):
    return obj({"schema_version": {"const": "0.1"}, "kind": {"const": kind}, **properties})


string = {"type": "string", "minLength": 1}
index = {"type": "integer", "minimum": 0}
number = {"type": "number", "minimum": -1.7976931348623157e308, "maximum": 1.7976931348623157e308}
indices = {**array(index), "uniqueItems": True}
parameter = obj({"id": string, "name": string, "dtype": {"const": "float64"}})
register = obj({"name": string, "bits": indices})
expression = {"oneOf": [obj({"op": {"const": "const"}, "value": number}),
                         obj({"op": {"const": "param"}, "id": string}),
                         obj({"op": {"enum": ["add", "mul", "pow", "sin", "cos", "tan", "exp", "log", "abs", "asin", "acos", "atan"]},
                              "args": {**array({"$ref": "#/$defs/expression"}), "minItems": 1}})]}
operation = obj({"name": {"enum": ["id","h","x","y","z","s","sdg","t","tdg","sx","rx","ry","rz","p","u3","cx","cz","swap","cp","measure","reset","barrier"]},
                 "qubits": {**indices, "minItems": 1}, "clbits": indices,
                 "params": array({"$ref": "#/$defs/expression"}), "source": {"type": "string"}})
schemas = {}
schemas["program"] = versioned("qllvm.program", {
    "name": string, "num_qubits": {"type": "integer", "minimum": 1}, "num_clbits": index,
    "parameters": array(parameter), "operations": array(operation),
    "global_phase": {"$ref": "#/$defs/expression"}, "registers": array(register),
    "source": obj({"frontend": string, "version": {"type":"string"}, "capture_scope": {"const":"circuit_only"}}),
})
schemas["program"]["$defs"] = {"expression": expression}
schemas["parameters"] = versioned("qllvm.parameters", {
    "binding_strategy": {"const": "specialize_before_codegen"}, "entries": array(parameter)})
schemas["target"] = versioned("qllvm.target", {
    "provider": {"enum": ["local", "mock"]}, "target": {"const": "basic-simulator"},
    "simulator": {"const": True}, "max_qubits": {"type":"integer", "minimum":1},
    "max_shots": {"type":"integer", "minimum":1}, "program_format": {"const":"openqasm2"},
    "parameter_binding": {"const":"specialize_before_codegen"}, "classical_control": {"const":False},
    "mid_circuit_measurement": {"const":True}, "reset": {"const":True}, "result_kind": {"const":"counts"},
})
schemas["results"] = versioned("qllvm.results", {
    "num_clbits": index, "bit_order": {"const":"c[n-1]...c[0]"}, "registers": array(register),
    "measurements": array(obj({"operation": index, "qubit": index, "clbit": index})),
    "unwritten_bits": {"const":"zero"}, "repeated_writes": {"const":"last_write_wins"},
})
sha = {"type":"string", "pattern":"^[a-f0-9]{64}$"}
schemas["executable"] = versioned("qllvm.executable", {
    "capture_scope": {"const":"circuit_only"}, "frontend_path": {"const":"sdk-object-to-mlir"},
    "parameter_binding": {"const":"specialize_before_codegen"}, "opt_level": {"enum":[0,1]},
    "compiler": obj({"path": string, "sha256": sha}), "executable_id": sha,
    "files": obj({name:sha for name in ["program.json","parameters.json","results.json","target.json"]}),
})
OUT.mkdir(exist_ok=True)
for name, schema in schemas.items():
    schema["$schema"] = "https://json-schema.org/draft/2020-12/schema"
    schema["title"] = "QLLVM " + name + " v0.1"
    (OUT / (name + ".schema.json")).write_text(json.dumps(schema, ensure_ascii=False, indent=2) + "\n")
print("Generated", len(schemas), "contracts in", OUT)
