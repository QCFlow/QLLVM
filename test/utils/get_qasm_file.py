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