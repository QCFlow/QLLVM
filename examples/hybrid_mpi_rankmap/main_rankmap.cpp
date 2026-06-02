/*
 * P5: per-rank kernel map — rank 0 runs bell, rank 1 runs vqe (local qir-runner).
 */
#include <iostream>
#include <cstdint>
#include <mpi.h>
#include "qir-runner-runtime.h"

extern "C" {
void __quantum__rt__initialize(int argc, int8_t** argv);
void __quantum__rt__finalize();
void qllvm_mpi_dispatch_by_rank(void);
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);
  int rank = 0, size = 1;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (size < 2) {
    if (rank == 0)
      std::cerr << "Need at least 2 MPI ranks for rank-kernel-map demo.\n";
    MPI_Finalize();
    return 1;
  }

  __quantum__rt__initialize(argc, reinterpret_cast<int8_t**>(argv));
  std::cout << "[rank " << rank << "] dispatch_by_rank()" << std::endl;
  qllvm_mpi_dispatch_by_rank();

  const int n = qir_runner_last_histogram_size();
  std::cout << "[rank " << rank << "] histogram entries: " << n << std::endl;
  for (int i = 0; i < n; ++i) {
    char bits[64];
    int count = 0;
    if (qir_runner_last_histogram_get(i, bits, sizeof(bits), &count) == 0)
      std::cout << "[rank " << rank << "] '" << bits << "': " << count << std::endl;
  }

  __quantum__rt__finalize();
  MPI_Finalize();
  return 0;
}
