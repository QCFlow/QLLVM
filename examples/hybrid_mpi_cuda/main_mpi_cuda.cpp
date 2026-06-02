/*
 * MPI + CUDA + quantum hybrid example (P2).
 */
#include <iostream>
#include <cstdint>

#include <mpi.h>
#include "qir-runner-runtime.h"

extern void launchCudaKernel(int n);

extern "C" {
void __quantum__rt__initialize(int argc, int8_t** argv);
void __quantum__rt__finalize();
void __internal_mlir_circuit(void);
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  int rank = 0;
  int size = 1;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (rank == 0) {
    std::cout << "MPI ranks: " << size << std::endl;
  }

  __quantum__rt__initialize(argc, reinterpret_cast<int8_t**>(argv));

  // Each rank runs CUDA (device context per process under mpirun).
  if (rank == 0) {
    std::cout << "CUDA kernel (all ranks):" << std::endl;
  }
  std::cout << "[rank " << rank << "] launching CUDA" << std::endl;
  launchCudaKernel(256);

  if (rank == 0) {
    std::cout << "Quantum circuit (rank 0 executes, histogram broadcast):" << std::endl;
  }
  __internal_mlir_circuit();

  const int n = qir_runner_last_histogram_size();
  for (int i = 0; i < n; ++i) {
    char bits[64];
    int count = 0;
    if (qir_runner_last_histogram_get(i, bits, sizeof(bits), &count) == 0) {
      std::cout << "[rank " << rank << "] '" << bits << "': " << count << std::endl;
    }
  }

  __quantum__rt__finalize();
  MPI_Finalize();
  return 0;
}
