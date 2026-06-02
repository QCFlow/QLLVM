/*
 * Exp-3: Distributed VQE-style outer loop — MPI classical reduction + root quantum kernel.
 */
#include <cmath>
#include <cstdint>
#include <iostream>
#include <mpi.h>
#include "qir-runner-runtime.h"

extern "C" {
void __quantum__rt__initialize(int argc, int8_t** argv);
void __quantum__rt__finalize();
void __internal_mlir_vqe(void);
}

static double estimateEnergyFromHistogram() {
  const int n = qir_runner_last_histogram_size();
  if (n <= 0) return 0.0;
  char bits[64];
  double e = 0.0;
  int total = 0;
  for (int i = 0; i < n; ++i) {
    int c = 0;
    if (qir_runner_last_histogram_get(i, bits, sizeof(bits), &c) != 0) continue;
    total += c;
    int ones = 0;
    for (int k = 0; bits[k]; ++k)
      if (bits[k] == '1') ++ones;
    e += (double)c * (double)ones;
  }
  return total > 0 ? e / (double)total : 0.0;
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);
  int rank = 0, size = 1;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  const int maxIter = 4;
  __quantum__rt__initialize(argc, reinterpret_cast<int8_t**>(argv));

  if (rank == 0)
    std::cout << "[mpi-vqe] ranks=" << size << " outer iterations=" << maxIter << "\n";

  double bestEnergy = 1e9;

  for (int it = 0; it < maxIter; ++it) {
    double localHint = (rank + 1) * 0.01 * (double)(it + 1);
    double globalHint = 0.0;
    MPI_Allreduce(&localHint, &globalHint, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

    __internal_mlir_vqe();

    double localE = estimateEnergyFromHistogram();
    double globalE = 0.0;
    MPI_Allreduce(&localE, &globalE, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    globalE /= (double)size;

    if (globalE < bestEnergy) bestEnergy = globalE;

    if (rank == 0) {
      std::cout << "[iter " << it << "] hint_sum=" << globalHint << " E_avg=" << globalE
                << " bestE=" << bestEnergy << "\n";
    }
  }

  __quantum__rt__finalize();
  MPI_Finalize();
  return 0;
}
