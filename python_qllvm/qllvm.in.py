# This code is part of QLLVM.
#
# (C) Copyright QCFlow 2026.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE file in the root directory
# of this source tree or at https://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.

from qiskit.qasm2 import dumps
from pyqpanda3.intermediate_compiler import convert_qprog_to_qasm,convert_originir_string_to_qprog
import cirq
import os

def compile(circuit,input,out_backend='gasm-backend', backend_config='', basicgate='', optlevel='0',output_path = '',state = False,error_print = False):
    qasm_str = ''
    if input == 'qasm' or input == 'qcis':
        qasm_str = circuit
    elif input =='qiskit':
        qasm_str = dumps(circuit)
    elif input == 'qprog':
        qasm_str = convert_qprog_to_qasm(circuit)
    elif input == 'originir':
        qasm_q = convert_originir_string_to_qprog(circuit)
        qasm_str = convert_qprog_to_qasm(qasm_q)
    elif input == 'cirq':
        qasm_str = cirq.qasm(circuit)
    else:
        raise ValueError(f"Backend {input} not supported")

    if qasm_str == '':
        raise ValueError(f"The circuit to be compiled is empty.")
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
    if error_print:
        compile_command += f" -fatal-error-verbose"
    os.system(compile_command)
    os.system(f"rm {origin_file}")