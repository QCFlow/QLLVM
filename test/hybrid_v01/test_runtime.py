from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import math
import os
import subprocess
import numpy as np
import pytest
from jsonschema import Draft202012Validator
from qiskit import QuantumCircuit, ClassicalRegister, QuantumRegister
from qiskit.circuit import Parameter
from qiskit.providers.basic_provider import BasicSimulator
from qiskit.quantum_info import Statevector
import cirq
import qllvm


def compiled_circuit(program,tmp_path,compiler,opt_level=0,bindings=None):
    bound=program.bind(bindings)
    inp=tmp_path/'program.json';out=tmp_path/'compiled.qasm'
    inp.write_text(json.dumps(bound.to_dict()))
    result=subprocess.run([compiler,str(inp),'-input-backend=program-json','-no-entrypoint',
                           '-internal-func-name=direct',f'-O{opt_level}','-exact-rotations',
                           '-emit-backend=qasm-backend','-qpu=qasm-backend',f'-output-path={out}'],
                          text=True,capture_output=True,timeout=90)
    assert result.returncode==0,result.stderr+result.stdout
    return QuantumCircuit.from_qasm_str(out.read_text()),result


@pytest.mark.parametrize('opt_level',[0,1])
def test_native_bell_wiring_and_unwritten_bits(tmp_path,compiler,opt_level):
    qc=QuantumCircuit(3,4);qc.x(2);qc.h(0);qc.cx(0,1);qc.barrier();qc.measure([0,1,2],[3,0,1])
    out,_=compiled_circuit(qllvm.import_program(qc),tmp_path,compiler,opt_level)
    counts=BasicSimulator().run(out,shots=512,seed_simulator=42).result().get_counts()
    assert set(counts)=={'0010','1011'}
    assert sum(counts.values())==512


@pytest.mark.parametrize('angle',[-0.7,0.4,1.3])
@pytest.mark.parametrize('opt_level',[0,1])
def test_native_parameter_binding_against_reference(tmp_path,compiler,angle,opt_level):
    t=Parameter('theta');qc=QuantumCircuit(2)
    qc.ry(2*t+0.1,0);qc.cx(0,1);qc.rz(t.sin(),1);qc.cp(0.37,0,1)
    out,_=compiled_circuit(qllvm.import_program(qc),tmp_path,compiler,opt_level,{'theta':angle})
    expected=Statevector.from_instruction(qc.assign_parameters({t:angle}))
    assert expected.equiv(Statevector.from_instruction(out))


def test_native_mid_measurement_and_reset(tmp_path,compiler):
    qc=QuantumCircuit(1,3);qc.x(0);qc.measure(0,0);qc.reset(0);qc.measure(0,1);qc.x(0);qc.measure(0,2)
    out,_=compiled_circuit(qllvm.import_program(qc),tmp_path,compiler)
    assert BasicSimulator().run(out,shots=20).result().get_counts()=={'101':20}


@pytest.mark.parametrize('bad',['version','unbound','unknown','alias','clbit','condition'])
def test_native_contract_failure_has_nonzero_exit(tmp_path,compiler,bad):
    qc=QuantumCircuit(2,1);qc.cx(0,1);qc.measure(0,0)
    data=qllvm.import_program(qc).to_dict()
    if bad=='version':data['schema_version']='9.0'
    elif bad=='unbound':data['parameters']=[{'id':'p0','name':'theta','dtype':'float64'}]
    elif bad=='unknown':data['operations'][0]['name']='unknown'
    elif bad=='alias':data['operations'][0]['qubits']=[0,0]
    elif bad=='clbit':data['operations'][-1]['clbits']=[1]
    else:data['operations'][0]['condition']=True
    inp=tmp_path/'bad.json';out=tmp_path/'bad.qasm';inp.write_text(json.dumps(data))
    result=subprocess.run([compiler,str(inp),'-input-backend=program-json','-no-entrypoint',
                           '-qpu=qasm-backend','-emit-backend=qasm-backend',f'-output-path={out}'],
                          text=True,capture_output=True,timeout=90)
    assert result.returncode!=0
    assert '[program-json]' in result.stderr
    assert not out.exists()


def test_bundle_schema_integrity_concurrent_results_and_resume(tmp_path,compiler):
    t=Parameter('theta');qc=QuantumCircuit(2,3);qc.ry(t,1);qc.measure(1,2)
    program=qllvm.import_program(qc)
    executable=qllvm.build(program,tmp_path/'circuit.qexe',compiler=compiler)
    schema_dir=Path(qllvm.__file__).parent/'qllvm_core/schemas'
    for name,file in [('program','program.json'),('parameters','parameters.json'),('results','results.json'),
                      ('target','target.json'),('executable','manifest.json')]:
        schema=json.loads((schema_dir/(name+'.schema.json')).read_text())
        Draft202012Validator.check_schema(schema)
        Draft202012Validator(schema).validate(json.loads((executable.path/file).read_text()))
    state=tmp_path/'jobs'
    with qllvm.LocalProvider(state) as provider:
        zero=provider.submit(executable,{'theta':0},shots=40,request_id='zero',seed=4)
        one=provider.submit(executable,{'theta':math.pi},shots=30,request_id='one',seed=4)
        assert zero.result(timeout=90).counts=={'000':40}
        assert one.result(timeout=90).counts=={'100':30}
        assert provider.submit(executable,{'theta':0},shots=40,request_id='zero',seed=4).job_id==zero.job_id
        with pytest.raises(qllvm.QLLVMError,match='REQUEST_CONFLICT'):
            provider.submit(executable,{'theta':0},shots=41,request_id='zero',seed=4)
        first_artifacts=list((state/'compiled').glob('*.qasm'))
        provider.submit(executable,{'theta':0},shots=5,request_id='cached').result(timeout=90)
        assert len(list((state/'compiled').glob('*.qasm')))==len(first_artifacts)==2
    with qllvm.LocalProvider(state) as provider:
        assert provider.resume('one').result(timeout=1).counts=={'100':30}
    with (executable.path/'program.json').open('a') as file:file.write(' ')
    with pytest.raises(qllvm.QLLVMError,match='BUNDLE_INTEGRITY'):qllvm.Executable(executable.path)


def test_cirq_key_counts_inversion_and_post_measurement(tmp_path,compiler):
    a,b=cirq.LineQubit.range(2)
    c=cirq.Circuit(cirq.X(a),cirq.measure(a,b,key='m',invert_mask=(True,False)),cirq.measure(a,key='post'))
    e=qllvm.build(qllvm.import_program(c,frontend='cirq'),tmp_path/'cirq.qexe',compiler=compiler)
    with qllvm.LocalProvider(tmp_path/'jobs') as p:
        r=p.submit(e,shots=50).result(timeout=90)
        assert r.register_counts('m')=={'00':50}
        assert r.register_counts('post')=={'1':50}
        assert r.counts=={'100':50}


def test_mock_identification_target_matching_and_failure(tmp_path,compiler):
    qc=QuantumCircuit(1,1);qc.x(0);qc.measure(0,0)
    mock=qllvm.build(qllvm.import_program(qc),tmp_path/'mock.qexe',compiler=compiler,
                     target=qllvm.TargetCapabilities(provider='mock'))
    with qllvm.MockProvider(tmp_path/'mock-jobs') as p:
        result=p.submit(mock,shots=10).result(timeout=90)
        assert result.provider=='mock' and result.simulated and result.counts=={'0':10}
    with qllvm.LocalProvider(tmp_path/'local-jobs') as p:
        with pytest.raises(qllvm.QLLVMError,match='TARGET_MISMATCH'):p.submit(mock)


def test_changed_compiler_and_output_collision(tmp_path,compiler):
    qc=QuantumCircuit(1,1);qc.measure(0,0)
    fake=tmp_path/'compiler';fake.write_text('#!/bin/sh\nexit 1\n');fake.chmod(0o755)
    e=qllvm.build(qllvm.import_program(qc),tmp_path/'fake.qexe',compiler=fake)
    with pytest.raises(qllvm.QLLVMError,match='OUTPUT_EXISTS'):
        qllvm.build(qllvm.import_program(qc),e.path,compiler=compiler)
    fake.write_text('#!/bin/sh\nexit 2\n')
    with qllvm.LocalProvider(tmp_path/'jobs') as p:
        with pytest.raises(qllvm.QLLVMError,match='COMPILER_CHANGED'):p.submit(e)


def test_failed_native_compilation_is_failed_job(tmp_path):
    qc=QuantumCircuit(1,1);qc.measure(0,0)
    fake=tmp_path/'compiler';fake.write_text('#!/bin/sh\necho injected-failure >&2\nexit 7\n');fake.chmod(0o755)
    e=qllvm.build(qllvm.import_program(qc),tmp_path/'bad.qexe',compiler=fake)
    with qllvm.LocalProvider(tmp_path/'jobs') as p:
        job=p.submit(e,request_id='failure')
        with pytest.raises(qllvm.QLLVMError,match='injected-failure'):job.result(timeout=10)
        assert job.status()=='FAILED'


def test_legacy_qasm_regression(tmp_path,compiler):
    source=tmp_path/'old.qasm';out=tmp_path/'old-out.qasm'
    source.write_text('OPENQASM 2.0;\ninclude "qelib1.inc";\nqreg q[2];\ncreg c[2];\nh q[0]; cx q[0],q[1]; measure q[0] -> c[0]; measure q[1] -> c[1];\n')
    result=subprocess.run([compiler,str(source),'-no-entrypoint','-qpu=qasm-backend','-emit-backend=qasm-backend',f'-output-path={out}'],capture_output=True,text=True,timeout=90)
    assert result.returncode==0,result.stderr+result.stdout
    counts=BasicSimulator().run(QuantumCircuit.from_qasm_str(out.read_text()),shots=100,seed_simulator=42).result().get_counts()
    assert set(counts)=={'00','11'}


@pytest.mark.parametrize('seed',range(8))
@pytest.mark.parametrize('opt_level',[0,1])
def test_native_random_circuits_and_identity_pairs(tmp_path,compiler,seed,opt_level):
    rng=np.random.default_rng(seed)
    qc=QuantumCircuit(3)
    for step in range(24):
        q=int(rng.integers(3));other=(q+1)%3
        kind=step%8
        if kind==0:qc.h(q);qc.h(q)
        elif kind==1:qc.rx(float(rng.uniform(-3,3)),q)
        elif kind==2:qc.cx(q,other);qc.cx(q,other)
        elif kind==3:qc.cp(float(rng.uniform(-3,3)),q,other)
        elif kind==4:qc.rz(float(rng.uniform(-3,3)),q)
        elif kind==5:qc.swap(q,other)
        elif kind==6:qc.s(q);qc.sdg(q)
        else:qc.u(*rng.uniform(-3,3,size=3),q)
    out,_=compiled_circuit(qllvm.import_program(qc),tmp_path,compiler,opt_level)
    from qiskit.quantum_info import Operator
    assert Operator(qc).equiv(Operator(out)),f'seed={seed}, O{opt_level}'


@pytest.mark.parametrize('opt_level',[0,1])
def test_reset_pairs_do_not_cross_measurements(tmp_path,compiler,opt_level):
    qc=QuantumCircuit(1,3);qc.x(0);qc.reset(0);qc.reset(0);qc.measure(0,0)
    qc.x(0);qc.measure(0,1);qc.reset(0);qc.measure(0,2)
    out,_=compiled_circuit(qllvm.import_program(qc),tmp_path,compiler,opt_level)
    assert BasicSimulator().run(out,shots=20).result().get_counts()=={'010':20}


@pytest.mark.parametrize('opt_level',[0,1])
def test_controlled_custom_phase_survives_native_pipeline(tmp_path,compiler,opt_level):
    from qiskit.quantum_info import Operator
    sub=QuantumCircuit(1,global_phase=.43);sub.ry(.7,0)
    qc=QuantumCircuit(2);qc.h(1);qc.append(sub.to_gate().control(ctrl_state=0),[1,0]);qc.h(1)
    out,_=compiled_circuit(qllvm.import_program(qc),tmp_path,compiler,opt_level)
    assert Operator(qc).equiv(Operator(out))


@pytest.mark.parametrize('opt_level',[0,1])
@pytest.mark.parametrize('case',['measurement_barrier','reset','repeated_write'])
def test_full_instrument_for_nonunitary_boundaries(tmp_path,compiler,opt_level,case):
    import sys
    sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'benchmark/official_suites'))
    from instrument_verification import verify_instrument
    qc=QuantumCircuit(1,2)
    if case=='measurement_barrier':
        qc.h(0);qc.measure(0,1);qc.h(0);qc.measure(0,0)
    elif case=='reset':
        qc.ry(.37,0);qc.reset(0);qc.reset(0);qc.h(0);qc.measure(0,1)
    else:
        qc.h(0);qc.measure(0,1);qc.x(0);qc.measure(0,1);qc.h(0)
    out,_=compiled_circuit(qllvm.import_program(qc),tmp_path,compiler,opt_level)
    result=verify_instrument(qc,out)
    assert result['status']=='passed',result


def test_cli_build_run_and_saved_result(tmp_path,compiler):
    import sys
    qc=QuantumCircuit(1,2);qc.x(0);qc.measure(0,1)
    source=tmp_path/'program.json';source.write_text(json.dumps(qllvm.import_program(qc).to_dict()))
    bundle=tmp_path/'cli.qexe';state=tmp_path/'jobs'
    commands=[['build',str(source),'-o',str(bundle),'--compiler',compiler],
              ['run',str(bundle),'--shots','8','--state-dir',str(state),'--request-id','cli-test'],
              ['status','cli-test','--state-dir',str(state)],
              ['result','cli-test','--state-dir',str(state)]]
    outputs=[]
    for command in commands:
        env=dict(os.environ,PYTHONPATH=str(Path(qllvm.__file__).parent)+os.pathsep+os.environ.get('PYTHONPATH',''))
        result=subprocess.run([sys.executable,'-m','qllvm_core',*command],text=True,capture_output=True,timeout=90,env=env)
        assert result.returncode==0,result.stderr+result.stdout
        outputs.append(json.loads(result.stdout))
    assert outputs[1]['counts']==outputs[3]['counts']=={'10':8}
    assert outputs[2]['status']=='SUCCEEDED'
