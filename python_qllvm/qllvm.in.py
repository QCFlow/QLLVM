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

def compile(circuit,input,out_backend='gasm-backend', backend_config='', basicgate='', optlevel='0',output_path = '',state = False):
    if input == 'qasm' or input == 'qcis':
        qasm_str = circuit
    elif input =='qiskit':
        qasm_str = dumps(circuit)
    elif input == 'qprog':
        qasm_str = convert_qprog_to_qasm(circuit)
    elif input == 'originir':
        qasm_q = convert_originir_string_to_qprog(circuit)
        qasm_str = convert_qprog_to_qasm(qasm_q)

    else:
        raise ValueError(f"Backend {input} not supported")

    current_dir = os.getcwd()
    origin_file = f"{current_dir}/_temp.qasm"
    with open(origin_file, 'w') as file:
        file.write(qasm_str)

    compile_command = f"qllvm {origin_file} -qrt nisq -qpu {out_backend} -O{optlevel}"
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