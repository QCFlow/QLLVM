import copy
import math
import numpy as np
import pytest
import sympy as sp
from qiskit import QuantumCircuit
from qiskit.circuit import Gate, Parameter
from qiskit.circuit.library import HGate, XGate, UGate
from qiskit.quantum_info import Operator
import cirq
import qllvm


def circuit_from_program(program, bindings=None):
    data = program.bind(bindings).to_dict()
    qc = QuantumCircuit(data["num_qubits"], data["num_clbits"])
    qc.global_phase = data["global_phase"]["value"]
    for op in data["operations"]:
        params = [p["value"] for p in op["params"]]
        if op["name"] == "u3":
            qc.u(*params, *op["qubits"])
        elif op["name"] == "measure":
            qc.measure(op["qubits"][0], op["clbits"][0])
        else:
            getattr(qc, op["name"])(*params, *op["qubits"])
    return qc


GATES = ["id","h","x","y","z","s","sdg","t","tdg","sx","rx","ry","rz","p","u","cx","cz","swap","cp"]


@pytest.mark.parametrize("name", GATES)
@pytest.mark.parametrize("angle", [-0.73, 0.4])
def test_qiskit_direct_gate_operator_and_phase(name, angle, monkeypatch):
    import qiskit.qasm2
    monkeypatch.setattr(qiskit.qasm2, "dumps", lambda *_: pytest.fail("QASM export was called"))
    qc = QuantumCircuit(2, global_phase=0.21)
    if name in ("rx","ry","rz","p"):
        getattr(qc, name)(angle, 1)
    elif name == "u":
        qc.u(angle, -0.2, 0.9, 1)
    elif name == "cp":
        qc.cp(angle, 1, 0)
    elif name in ("cx","cz","swap"):
        getattr(qc, name)(1, 0)
    else:
        getattr(qc, name)(1)
    imported = qllvm.import_program(qc)
    assert imported.capture_scope == "circuit_only"
    np.testing.assert_allclose(Operator(circuit_from_program(imported)).data, Operator(qc).data, atol=1e-12)


@pytest.mark.parametrize("value", [-0.9, 0.0, 0.63])
def test_symbolic_expression_and_controlled_custom_phase(value):
    t = Parameter("θ")
    sub = QuantumCircuit(1, global_phase=0.37)
    sub.ry(t.sin() + t / 3, 0)
    qc = QuantumCircuit(2)
    qc.append(sub.to_gate().control(ctrl_state=0), [1, 0])
    imported = qllvm.import_program(qc)
    restored = circuit_from_program(imported, {"θ": value})
    reference = qc.assign_parameters({t: value})
    np.testing.assert_allclose(Operator(restored).data, Operator(reference).data, atol=1e-10)


def test_custom_gate_cannot_spoof_standard_name():
    fake_x = Gate("x", 1, [])
    fake_x.definition = QuantumCircuit(1)
    fake_x.definition.h(0)
    qc = QuantumCircuit(1); qc.append(fake_x, [0])
    imported = qllvm.import_program(qc)
    assert imported.to_dict()["operations"][0]["name"] == "h"
    np.testing.assert_allclose(Operator(circuit_from_program(imported)).data, Operator(HGate()).data)


def test_import_is_snapshot_and_retains_measurement_wiring():
    qc = QuantumCircuit(3, 4); qc.x(1); qc.measure([2, 0, 1], [0, 3, 2])
    p = qllvm.import_program(qc)
    fingerprint = p.fingerprint
    qc.x(0); qc.measure(0,1)
    data = p.to_dict(); data["operations"].clear()
    assert p.fingerprint == fingerprint
    assert [(m["qubit"],m["clbit"]) for m in p.result_contract()["measurements"]] == [(2,0),(0,3),(1,2)]


@pytest.mark.parametrize("bad", ["control", "delay", "unknown", "limit"])
def test_unsupported_semantics_fail_with_location(bad):
    qc = QuantumCircuit(1,1)
    kwargs = {}
    if bad == "control":
        with qc.if_test((qc.clbits[0],1)): qc.x(0)
    elif bad == "delay":
        qc.delay(10,0)
    elif bad == "unknown":
        qc.append(Gate("invented",1,[]),[0])
    else:
        qc.x(0); qc.x(0); kwargs["max_operations"] = 1
    with pytest.raises(qllvm.QLLVMError) as error:
        qllvm.import_program(qc, **kwargs)
    assert "circuit.data[" in str(error.value)


@pytest.mark.parametrize("value", [-0.6,0.3,1.0])
@pytest.mark.parametrize("kind", ["x","y","z","h","cx","cz","swap"])
def test_cirq_power_global_shift_and_order(kind,value,monkeypatch):
    monkeypatch.setattr(cirq,"qasm",lambda *_: pytest.fail("Cirq QASM export was called"))
    a,b = cirq.LineQubit.range(2)
    t = sp.Symbol("t")
    if kind in ("x","y","z"):
        cls={"x":cirq.XPowGate,"y":cirq.YPowGate,"z":cirq.ZPowGate}[kind]
        gate=cls(exponent=t,global_shift=-0.27)
        ops=[gate(a),cirq.I(b)]
        bindings={"t":value}
    else:
        gate={"h":cirq.H,"cx":cirq.CNOT,"cz":cirq.CZ,"swap":cirq.SWAP}[kind]
        ops=[gate(a) if kind=="h" else gate(a,b),cirq.I(b)]
        bindings={}
    circuit=cirq.Circuit(ops)
    program=qllvm.import_program(circuit,frontend="cirq")
    restored=circuit_from_program(program,bindings)
    expected=cirq.resolve_parameters(circuit,bindings).unitary(qubit_order=[b,a])
    np.testing.assert_allclose(Operator(restored).data,expected,atol=1e-12)


def test_cirq_invert_mask_preserves_post_measurement_state():
    a=cirq.LineQubit(0)
    c=cirq.Circuit(cirq.X(a),cirq.measure(a,key="first",invert_mask=(True,)),cirq.measure(a,key="second"))
    p=qllvm.import_program(c,frontend="cirq")
    names=[op["name"] for op in p.to_dict()["operations"]]
    assert names[-4:]==["x","measure","x","measure"]


def test_cirq_feedback_and_duplicate_keys_rejected():
    q=cirq.LineQubit(0)
    for c in (cirq.Circuit(cirq.measure(q,key="m"),cirq.X(q).with_classical_controls("m")),
              cirq.Circuit(cirq.measure(q,key="m"),cirq.measure(q,key="m"))):
        with pytest.raises(qllvm.QLLVMError): qllvm.import_program(c,frontend="cirq")


@pytest.mark.parametrize("bindings", [{},{"theta":float("nan")},{"theta":True},{"theta":1,"extra":0}])
def test_bad_bindings_rejected(bindings):
    t=Parameter("theta");c=QuantumCircuit(1);c.rx(t,0)
    with pytest.raises(qllvm.QLLVMError): qllvm.import_program(c).bind(bindings)


def test_expression_domain_error():
    t=Parameter("theta");c=QuantumCircuit(1);c.rx(t.log(),0)
    with pytest.raises(qllvm.QLLVMError,match="PARAMETER_DOMAIN"):
        qllvm.import_program(c).bind({"theta":-1})


@pytest.mark.parametrize("mutation", ["version","index","alias","unknown","extra","expression"])
def test_contract_rejects_invalid_payload(mutation):
    qc=QuantumCircuit(2,1);qc.cx(0,1);qc.measure(0,0)
    data=qllvm.import_program(qc).to_dict()
    if mutation=="version": data["schema_version"]="99"
    elif mutation=="index": data["operations"][0]["qubits"]=[0,2]
    elif mutation=="alias": data["operations"][0]["qubits"]=[0,0]
    elif mutation=="unknown": data["operations"][0]["name"]="unimplemented"
    elif mutation=="extra": data["operations"][0]["condition"]=True
    else: data["global_phase"]={"op":"eval","args":[]}
    with pytest.raises(qllvm.QLLVMError): qllvm.Program(data)
