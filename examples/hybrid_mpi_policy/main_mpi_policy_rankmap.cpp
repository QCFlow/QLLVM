/*
 * Exp-2b: rank-kernel-map variant — each rank runs a different policy kernel locally.
 */
#include <iostream>
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

  if (size < 4) {
    if (rank == 0)
      std::cerr << "Need mpirun -np 4 for rank-map policy demo (0:policy_0 .. 3:policy_3).\n";
    MPI_Finalize();
    return 1;
  }

  __quantum__rt__initialize(argc, reinterpret_cast<int8_t**>(argv));
  std::cout << "[rank " << rank << "] qllvm_mpi_dispatch_by_rank()" << std::endl;
  qllvm_mpi_dispatch_by_rank();

  const int n = qir_runner_last_histogram_size();
  std::cout << "[rank " << rank << "] histogram entries=" << n << std::endl;

  __quantum__rt__finalize();
  MPI_Finalize();
  return 0;
}
