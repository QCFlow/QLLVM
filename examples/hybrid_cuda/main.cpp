/*
 * This code is part of QLLVM.
 *
 * (C) Copyright QCFlow 2026.
 *
 * This code is licensed under the Apache License, Version 2.0. You may
 * obtain a copy of this license in the LICENSE file in the root directory
 * of this source tree or at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * Any modifications or derivative works of this code must retain this
 * copyright notice, and modified files need to carry a notice indicating
 * that they have been altered from the originals.
 *
 * Modified by QCFlow (2026) for QLLVM project.
 */

#include <iostream>
#include <cstdint>

// CUDA host interface (provided by kernel.cu)
extern void launchCudaKernel(int n);

// QIR runtime and quantum kernel
extern "C" {
void __quantum__rt__initialize(int argc, int8_t** argv);
void __quantum__rt__finalize();
void __internal_mlir_circuit(void);  // from circuit.qasm
}

int main(int argc, char** argv) {
  __quantum__rt__initialize(argc, reinterpret_cast<int8_t**>(argv));

  launchCudaKernel(1024);      // call CUDA
  __internal_mlir_circuit();   // call quantum

  __quantum__rt__finalize();
  return 0;
}
