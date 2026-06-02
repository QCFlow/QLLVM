/*
 * P4: MPI + llvm-link in-process QIR (no qir-runner subprocess).
 */
#include <iostream>
#include <cstdint>
#include <mpi.h>

extern "C" {
void __quantum__rt__initialize(int argc, int8_t** argv);
void __quantum__rt__finalize();
void __internal_mlir_bell(void);
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);
  int rank = 0, size = 1;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (rank == 0)
    std::cout << "MPI llvm-link mode, ranks=" << size << std::endl;

  __quantum__rt__initialize(argc, reinterpret_cast<int8_t**>(argv));
  if (rank == 0)
    std::cout << "Calling in-process __internal_mlir_bell()" << std::endl;
  __internal_mlir_bell();
  if (rank == 0)
    std::cout << "Done (qir-qrt-stub no-op runtime)." << std::endl;

  __quantum__rt__finalize();
  MPI_Finalize();
  return 0;
}
