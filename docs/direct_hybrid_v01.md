# 直接前端与本地混合执行 v0.1

本轮新增 Qiskit/Cirq 对象 → 版本化 Program → C++ 直接构建 quantum MLIR → 现有优化与 QASM 指令后端 → LocalProvider → 独立 Job/Result 的闭环。导入和 MLIR 构建不经过 QASM 序列化/解析；本地模拟器读取的 QASM 是编译后的目标产物。

Python 主机循环可以重复绑定参数并获取结果。本轮采用绑定后特化，再运行旧优化遍；`.qexe` 保存可重用的参数模板和编译器指纹，相同绑定复用已验证哈希的本地编译缓存。它没有声称设备端自由参数绑定，也没有捕获或编译外层 Python 工作流。

## 使用

在源码根目录安装 Python 包；构建和运行分别使用显式指定的原生编译器和 Python 环境。可用 `PYTHONPATH=$PWD/python_qllvm` 直接运行，也可在独立环境 `python -m pip install './python_qllvm[qiskit,cirq,local,test]'`。验证环境固定版本见 `python_qllvm/requirements-test.lock`，完整实际环境随验证记录留存。

```python
import qllvm
from qiskit import QuantumCircuit
from qiskit.circuit import Parameter

theta = Parameter("theta")
qc = QuantumCircuit(2, 2)
qc.ry(theta, 0)
qc.cx(0, 1)
qc.measure([0, 1], [1, 0])

program = qllvm.import_program(qc, frontend="qiskit")
executable = qllvm.build(program, "ansatz.qexe", compiler="/path/to/rebuilt/qllvm-compile")
with qllvm.LocalProvider("./job-records") as provider:
    job = provider.submit(executable, {"theta": 0.8}, shots=1024, seed=42)
    result = job.result(timeout=120)
    print(job.job_id, result.counts, result.provider, result.simulated)
```

完整可运行主机参数扫描见 `examples/direct_hybrid/parameter_sweep.py`。输出目录已存在时 build 拒绝覆盖。

Program 可用 `program.to_dict()` 保存为 JSON，然后使用独立命令：

```bash
python -m qllvm_core build program.json -o ansatz.qexe --compiler /path/to/rebuilt/qllvm-compile
python -m qllvm_core run ansatz.qexe --bindings '{"theta":0.8}' --shots 1024 --state-dir ./job-records
python -m qllvm_core status JOB_ID --state-dir ./job-records
python -m qllvm_core result JOB_ID --state-dir ./job-records
```

CMake 安装后的 `qllvm build/run/status/result` 分发到相同接口。原来的文件编译入口保留；原 `qllvm.compile(...)` 作为文本兼容包装保留，按所选前端延迟导入 SDK，临时文件隔离并检查子进程退出状态。

## 支持范围

- Qiskit 2.6.1、Cirq 1.6.1 经本轮验证；其他版本需单独验证，包的依赖范围不等于所有版本均已测试。
- 基础单/双量子门、标量实参数表达式、测量、reset、barrier（Qiskit）和有限深度复合门分解。
- Program 保存总相位；Cirq 幂门的 `global_shift` 转换计入相位。Qiskit 受控自定义门通过其定义分解，保留控制相位；不按门名猜测未知门语义。
- 不支持的动态控制流、时序操作、经典变量、Cirq CircuitOperation/标签、噪声通道、重复 Cirq 测量 key 等明确报错，不隐式退回另一种格式。
- ProgramBuilder 是 SDK 独立的构造和批量追加接口。v0.1 跨 Python/C++ 边界为版本化 JSON；尚未交付进程内 C Builder ABI、高层工作流捕获或现代 LLVM 迁移。
- 新 Runtime 只开放 QASM 采样目标，本地模拟器上限20量子位、单次100万shots；动态分支和物理路由没有纳入该执行合同。
- 直接入口的 O1 仅启用已验证的局部门对消去与重复reset删除。首轮发现的旧CP矩阵符号错误已修复并加入回归；完整综合序列仍需逐遍验证后再向新入口开放。旧QASM入口沿原流水线执行并使用修复后的CP矩阵。

## 数据合同与任务语义

五种 v0.1 JSON Schema 位于 `python_qllvm/qllvm_core/schemas`：Program、参数 ABI、目标能力、结果合同、Executable。Schema 描述结构，Python/C++验证器还检查索引、门参数数量、参数引用、资源别名等语义约束。修改生成脚本后需运行 `python scripts/generate_hybrid_schemas.py` 更新文件。

结果 counts 的最右位为 c[0]，保留未写入位为0，重复写入取最后一次值。源寄存器及每次测量接线单独保存；`result.register_counts(name)` 根据源寄存器定义投影。Cirq 一个测量key的首个被测量子位对应其key整数的最高位，适配器已保存该顺序。

Job分别持有结果，任务记录持久化到state目录。request_id相同且输入相同返回已有任务，输入不同报冲突，不自动重复执行。编译失败/结果错误标记FAILED。LocalProvider取消仅支持尚未开始的任务；调用result超时不会取消运行中的任务。进程退出后可重新读取已完成任务；失去worker的未完成任务标记SUBMIT_UNKNOWN，不自动重提交。MockProvider的结果明确标为mock和simulated，供协议测试使用。

本地 Provider 采用工作线程，`close()` 等待本进程任务完成。CLI run 同步等待后返回；它不是后台守护服务。会话、云端排队、跨机器恢复、厂商Provider及真机执行尚未交付，待首家可用平台落实后接入。

## 构建和验证

```bash
cmake -S . -B /path/to/build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ROOT=/path/to/llvm12 -DLLVM_DIR=/path/to/llvm12/lib/cmake/llvm \
  -DMLIR_DIR=/path/to/llvm12/lib/cmake/mlir -DQLLVM_ENABLE_MPI=OFF
cmake --build /path/to/build --target qllvm-compile -j 2
QLLVM_COMPILER=/path/to/build/bin/qllvm-compile python -m pytest test/hybrid_v01 -q
```

原生命令的直接导入入口为 `qllvm-compile program.json -input-backend=program-json -no-entrypoint -qpu=qasm-backend -emit-backend=qasm-backend -output-path=compiled.qasm`，输入必须先绑定为实常量。直接入口复用优化/降级，但保持外层主机程序由Runtime管理。总相位保存在模板及导入MLIR中，QASM2采样输出不编码该总相位；它不影响本次采样合同。

测试包括源SDK参考矩阵/状态对比、控制相位、参数域、位序、复位、重复测量、并发任务隔离、缓存复用、幂等冲突、产物篡改、编译失败与旧QASM入口回归。记录构建来源可使用 `scripts/capture_hybrid_build.py --compiler /path/to/compiler --output /path/to/evidence`；同时保存CMake配置、构建和测试日志，外部系统依赖仍需单独锁定。
