from codecs import escape_encode
import sys, uuid, atexit, hashlib

import typing, types
import re
import itertools
from collections import defaultdict
import os
from qiskit import QuantumCircuit,transpile
from qiskit.qasm2 import dumps
from qiskit_aer import Aer
from pyqpanda3.intermediate_compiler import convert_qprog_to_qasm,convert_originir_string_to_qprog

def compile(circuit,in_backend,out_backend='gasm-backend', backend_config='', basicgate='', optlevel='0',output_path = '',state = False):
    if in_backend == 'qasm' or in_backend == 'qcis':
        qasm_str = circuit
    elif in_backend =='qiskit':
        qasm_str = dumps(circuit)
    elif in_backend == 'qprog':
        qasm_str = convert_qprog_to_qasm(circuit)
    elif in_backend == 'originir':
        qasm_q = convert_originir_string_to_qprog(circuit)
        qasm_str = convert_qprog_to_qasm(qasm_q)

    else:
        raise ValueError(f"Backend {in_backend} not supported")

    current_dir = os.getcwd()
    origin_file = f"{current_dir}/_temp.qasm"
    with open(origin_file, 'w') as file:
        file.write(qasm_str)

    compile_command = f"qcc {origin_file} -qrt nisq -qpu {out_backend} -O{optlevel}"
    if backend_config:
        compile_command += f" -qpu-config {backend_config}"
    if basicgate:
        compile_command += f" -basicgate={basicgate}"
    if output_path:
        base_path = os.path.splitext(output_path)[0]
        compile_command += f" -o {base_path}"
    if state:
        compile_command += f" -circuit-state"
    os.system(compile_command)
    os.system(f"rm {origin_file}")
    if output_path:
        print(f"The compiled file has been saved to: {output_path}")
    else:
        if out_backend in {"tianyan","originquantum"}:
            output_path = f"{current_dir}/_temp_compiled.py"
        else:
            output_path = f"{current_dir}/_temp_compiled.qasm"

        print(f"The compiled file has been saved to: {output_path}")