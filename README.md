<div align="center">

<!-- Dynamic gradient background effect (implemented with SVG) -->
<img src="docs/image/QLLVM0.0.png" alt="QLLVM Logo" width="100%"/>

## QLLVM Quantum Compilation Framework


<p align="center">
  <a href="README.md">English</a> • 
  <a href="README.cn.md">中文</a>
</p>

<p align="center">
  <a href="https://openreview.net/forum?id=5N3z9JQJKq"><img src="https://img.shields.io/badge/ICLR-2026-8A2BE2?style=for-the-badge&logo=openreview&logoColor=white&color=6f2da8" alt="ICLR 2026"></a>
  <a href="https://www.apache.org/licenses/LICENSE-2.0"><img src="https://img.shields.io/badge/License-Apache%202.0-D22128?style=for-the-badge&logo=apache&logoColor=white" alt="License: Apache 2.0"></a>
  <a href="https://www.python.org/"><img src="https://img.shields.io/badge/Python-3.10+-3776AB?style=for-the-badge&logo=python&logoColor=white" alt="Python"></a>
  
</p>

</div>

---

**QLLVM** is a classical-quantum hybrid compilation framework built on [LLVM](https://llvm.org/), with excellent **extensibility** and seamless integration with the **classical high-performance computing ecosystem**.

QLLVM supports multiple quantum programming languages and backends, including Qiskit and OpenQASM, with target backends such as qasm simulators, native quantum computers, and China Telecom’s “Tianyan” quantum computer.

QLLVM supports unified compilation of quantum programs, CUDA programs, and classical C++ programs, providing an efficient, flexible, industrial-grade compilation infrastructure for future classical-quantum software development.

For more details on how to use QLLVM, please refer to the documentation here: [QLLVM Documentation](https://qllvm-documentation.readthedocs.io/en/latest/index.en.html)

***

### 🚀 Installation

##### Cloud: Quick installation via VSCode extensions, providing intelligent programming, compilation, and execution services

```bash
1. Download the repository to obtain the plugin installation packages:
    `./plugin/quantum-circuit-composer-0.1.vsix`
    `./plugin/qcoder-0.1.vsix`
2. Open the command palette in VSCode (`Ctrl+Shift+P` / `Cmd+Shift+P`)
3. Enter and select **Extensions: Install from VSIX...**
4. Select the downloaded `.vsix` files in sequence to complete installation
```

##### Local: Install the QLLVM compiler from source

> Please refer to the [Source Installation Guide](https://qllvm-documentation.readthedocs.io/en/latest/installation.en.html#installation-from-source)

---

### Cloud Usage
<div align="center">

<img src="docs/image/006.png" alt="QLLVM Logo" width="100%"/>

VSCode plugin interface display

</div>


| Area | Description |
|:----:|----------|
| **① Qcoder Sidebar** | Click to use the Qcoder intelligent programming assistant |
| **② Qcoder Main Interface** | Intelligent interaction interface |
| **③ Code Panel** | Displays quantum programs |
| **④ Compile Button** | Click to compile the code panel or the selected quantum program |
| **⑤ Run Button** | Click to run the code panel or the selected quantum program |
| **⑥ Output Panel** | Displays the compiled quantum circuit and related parameters |


> 📚 For detailed plugin usage instructions, please refer to the [Plugin Documentation](https://qllvm-documentation.readthedocs.io/en/latest/usage.en.html#using-plugins)

---

### Local Command Line Usage

After installation, you can use the `qllvm` command in the command line to compile classical-quantum hybrid programs or pure quantum programs.

#### 🔹 Compile a pure quantum program

```bash
qllvm test/test_bell.qasm -qrt nisq -qpu qasm-backend -O1
```
This command compiles and generates `test/test_bell_compiled.qasm`
>📖 For complete compilation parameter reference, please refer to the [qllvm User Guide](https://qllvm-documentation.readthedocs.io/en/latest/usage.en.html#compilation-parameter-explanation)

#### 🔹 Compile a classical-quantum hybrid program
Compile a C++ program and a QASM quantum circuit together

```bash
qllvm examples/hybrid/main.cpp examples/hybrid/bell.qasm -o hybrid_bell
./hybrid_bell
```
Compile a C++ main program, CUDA kernel, and QASM quantum circuit together; execution requires a CUDA environment

```bash
cd examples/hybrid_cuda
qllvm main.cpp kernel.cu circuit.qasm -o hybrid_app \
      -cuda-arch sm_75 \
      -cuda-path /usr/local/cuda
qllvm main.cpp kernel.cu circuit.qasm -o hybrid_app -cuda-arch sm_86
./hybrid_app -shots 1024
```

>📖 For detailed usage of different programming languages and backends, please refer to the [Usage Documentation](https://qllvm-documentation.readthedocs.io/en/latest/usage.en.html)

---

### 📚 QLLVM Documentation

For detailed QLLVM descriptions, installation, and usage documentation, please consult the online documentation:

| Document | Description |
|------|------|
| 📖 [Learn about the QLLVM Quantum Compilation Framework](https://qllvm-documentation.readthedocs.io/en/latest/introduction.en.html) | Get detailed introduction and design philosophy of QLLVM |
| 🔧 [Install QLLVM and related plugins](https://qllvm-documentation.readthedocs.io/en/latest/installation.en.html) | Complete installation and configuration guide |
| 🎓 [Learn how to use it](https://qllvm-documentation.readthedocs.io/en/latest/usage.en.html) | Tutorials and examples to get started quickly |

---

### 🤝 Contribution Guide

If you want to contribute to QLLVM, please read the [Contribution Guide](https://qllvm-documentation.readthedocs.io/en/latest/contributing.en.html). By participating, you agree to follow our [Code of Conduct](https://qllvm-documentation.readthedocs.io/en/latest/contributing.en.html#code-of-conduct).

We use [Issues](https://github.com/QCFlow/QLLVM/issues) for issue tracking and feature requests.

---

### 📝 Authors and Citation

QLLVM is the result of the joint effort of many contributors. If you use QLLVM, please refer to [How to Contribute](https://qllvm-documentation.readthedocs.io/en/latest/contributing.en.html#how-to-contribute) for citation.

---

### 📅 Changelog

Release notes for each version are dynamically generated on the GitHub Releases page. 

---

### 🙏 Acknowledgments

This project is built on the **MLIR** and **LLVM** ecosystems. Thanks to the open source community for their contributions. Special thanks to Origin Quantum, China Telecom Tianyan, Medical Image Bio, Beijing Institute of Quantum Electronics, Zhejiang University, Auroral Quantum, and other organizations for their support of this project.

---

### 📄 License

This project is open source under the [Apache 2.0](LICENSE) license.
