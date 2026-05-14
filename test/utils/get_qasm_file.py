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


def get_qasm_files(folder_path):
    qasm_files = []
    
    for root, dirs, files in os.walk(folder_path):

        for file in files:
            if file.endswith(".qasm"):
                file_path = os.path.join(root, file)
                qasm_files.append(file_path)
    
        for dir in dirs:
            dir_path = os.path.join(root, dir)
            qasm_files.extend(get_qasm_files(dir_path))
    
    return sorted(qasm_files)