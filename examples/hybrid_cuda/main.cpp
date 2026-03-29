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
