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

#include "qir-runner-runtime.h"

// Quantum kernel from bell.qasm (__internal_mlir_<stem>)
extern "C" {
void __quantum__rt__initialize(int argc, int8_t** argv);
void __quantum__rt__finalize();
void __internal_mlir_bell(void);
}

int main(int argc, char** argv) {
  int N = 0;
  std::cout << "Classical computing result!" << std::endl;
  std::cout << "Please input the number of N,to compute 1+..+N:\n";
  std::cin >> N;
  // std::cout << "The number of qubits is: " << N << std::endl;
  int sum = 0;
  for(int i = 1; i <= N; i++) {
    sum += i;
  }
  std::cout << "The result is: " << sum << std::endl;

  __quantum__rt__initialize(argc, reinterpret_cast<int8_t**>(argv));

  std::cout << "Quantum circuit (qir-runner) output:" << std::endl;

  __internal_mlir_bell();  // runs qir-runner; prints counts + fills histogram API

  std::cout << "Histogram in C++:" << std::endl;
  int n = qir_runner_last_histogram_size();
  for (int i = 0; i < n; ++i) {
    char bits[64];
    int c = 0;
    if (qir_runner_last_histogram_get(i, bits, sizeof(bits), &c) == 0)
      std::cout << "  '" << bits << "': " << c << std::endl;
  }

  __quantum__rt__finalize();
  std::cout << "Done." << std::endl;


  return 0;
}
