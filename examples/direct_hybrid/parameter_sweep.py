"""A Python host loop over one imported quantum parameter template.

Run with the rebuilt compiler: python parameter_sweep.py --compiler /path/qllvm-compile
"""
import argparse
import json
import math
from pathlib import Path
import tempfile
from qiskit import QuantumCircuit
from qiskit.circuit import Parameter
import qllvm


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--compiler',required=True)
    args=parser.parse_args()
    theta=Parameter('theta')
    circuit=QuantumCircuit(2,2)
    circuit.ry(theta,0);circuit.cx(0,1);circuit.measure([0,1],[0,1])
    program=qllvm.import_program(circuit)
    with tempfile.TemporaryDirectory(prefix='qllvm-sweep-') as temporary:
        root=Path(temporary)
        executable=qllvm.build(program,root/'ansatz.qexe',compiler=args.compiler)
        with qllvm.LocalProvider(root/'jobs') as provider:
            history=[]
            for value in (0.0,math.pi/2,math.pi,0.0):
                job=provider.submit(executable,{'theta':value},shots=1024,seed=42)
                result=job.result(timeout=90)
                z0=sum((1 if bits[-1]=='0' else -1)*count for bits,count in result.counts.items())/result.shots
                history.append({'theta':value,'z0':z0,'counts':result.counts,'job_id':job.job_id})
            print(json.dumps({'capture_scope':program.capture_scope,
                              'binding_strategy':executable.manifest['parameter_binding'],
                              'provider':'local','simulated':True,'evaluations':history,
                              'native_specializations':len(list((root/'jobs/compiled').glob('*.qasm')))},indent=2))


if __name__=='__main__':main()
