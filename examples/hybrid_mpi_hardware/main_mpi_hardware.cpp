/*
 * P3: MPI hardware path with mock MPIQ runtime (no real QPU).
 */
#include <iostream>
#include <mpi.h>

extern "C" {
void __internal_mlir_bell(void);
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0)
    std::cout << "MPI hardware (mock MPIQ) dispatch" << std::endl;

  __internal_mlir_bell();

  MPI_Finalize();
  return 0;
}
