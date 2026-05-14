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

import os
from qiskit import QuantumCircuit
import csv
from collections import defaultdict
import pandas as pd

def get_metrics(file_path):
    try:
        qc = QuantumCircuit.from_qasm_file(file_path)
        return {
            'gate_count': qc.size(),
            'depth': qc.depth(),
            'error': None
        }
    except Exception as e:
        return {'error': str(e)}
    
def compare_qasm_diff(dir1, dir2,output_csv):
    files1 = {f for f in os.listdir(dir1) if f.endswith('.qasm')}
    files2 = {f for f in os.listdir(dir2) if f.endswith('.qasm')}
    common_files = sorted(files1 & files2)

    if not common_files:
        print("error: no common files found")
        return

    csv_header = ['file name', 'Qiskit gate count', 'QCC gate count', 'gate count ratio', 
                 'Qiskit depth', 'QCC depth', 'depth ratio']
    
    with open(output_csv, 'w', newline='', encoding='utf-8') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(csv_header) 
        for file in common_files:
            qiskit = get_metrics(os.path.join(dir1, file))
            qcc = get_metrics(os.path.join(dir2, file))

            if qiskit['error'] or qcc['error']:
                error_msg = qiskit['error'] or qcc['error']
                writer.writerow([file, 'ERROR', 'ERROR', error_msg])
                continue

            gate_ration = (1 - qcc['gate_count'] / qiskit['gate_count']) * 100
            depth_ration = (1 - qcc['depth'] / qiskit['depth']) * 100

            writer.writerow([
                file,
                qiskit['gate_count'],
                qcc['gate_count'],
                gate_ration,
                qiskit['depth'],
                qcc['depth'],
                depth_ration
            ])
