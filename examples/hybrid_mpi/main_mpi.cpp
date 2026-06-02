/*
 * MPI + classical + quantum hybrid example (P0).
 * Compile: qllvm main_mpi.cpp bell.qasm -o mpi_bell -qpu qir-runner -mpi
 * Run:     mpirun -np 4 ./mpi_bell -shots 512
 */
#include <iostream>
#include <cstdint>

#include <mpi.h>
#include "qir-runner-runtime.h"

extern "C" {
void __quantum__rt__initialize(int argc, int8_t** argv);
void __quantum__rt__finalize();
void __internal_mlir_bell(void);
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  int rank = 0;
  int size = 1;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int local_sum = rank + 1;
  int global_sum = 0;
  MPI_Allreduce(&local_sum, &global_sum, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

  if (rank == 0) {
    std::cout << "MPI ranks: " << size << ", classical Allreduce sum(1..N) = "
              << global_sum << std::endl;
  }

  __quantum__rt__initialize(argc, reinterpret_cast<int8_t**>(argv));

  if (rank == 0) {
    std::cout << "Quantum circuit (rank 0 executes, histogram broadcast):" << std::endl;
  }

  __internal_mlir_bell();

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
