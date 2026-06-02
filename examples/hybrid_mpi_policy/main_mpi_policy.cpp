/*
 * Exp-2: MPI + CUDA + multi-policy quantum control (Layout A: root QPU + Bcast).
 * Based on examples/hybrid_cuda_strong — quantum action drives HPC solver knobs.
 */
#include <atomic>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <mpi.h>
#include <string>
#include <thread>

#include "qir-runner-runtime.h"

extern "C" {
void __quantum__rt__initialize(int argc, int8_t** argv);
void __quantum__rt__finalize();
uint64_t hpc_stencil2d_step(int nx, int ny, int subdomains, int innerIters, int seed, double omega,
                            double* resid_out);
void __internal_mlir_policy_0(void);
void __internal_mlir_policy_1(void);
void __internal_mlir_policy_2(void);
void __internal_mlir_policy_3(void);
void __internal_mlir_policy_4(void);
void __internal_mlir_policy_5(void);
void __internal_mlir_policy_6(void);
void __internal_mlir_policy_7(void);
}

static int readIntArg(int argc, char** argv, const char* name, int def) {
  const std::string prefix = std::string("--") + name + "=";
  for (int i = 1; i < argc; ++i) {
    if (!argv[i]) continue;
    std::string s(argv[i]);
    if (s.rfind(prefix, 0) == 0) return std::atoi(s.substr(prefix.size()).c_str());
  }
  return def;
}

static void runPolicyKernel(int id) {
  switch (id & 7) {
    case 0: __internal_mlir_policy_0(); break;
    case 1: __internal_mlir_policy_1(); break;
    case 2: __internal_mlir_policy_2(); break;
    case 3: __internal_mlir_policy_3(); break;
    case 4: __internal_mlir_policy_4(); break;
    case 5: __internal_mlir_policy_5(); break;
    case 6: __internal_mlir_policy_6(); break;
    default: __internal_mlir_policy_7(); break;
  }
}

static int histogramArgmaxAction3bit() {
  const int n = qir_runner_last_histogram_size();
  if (n <= 0) return 0;
  char bits[64];
  int bestCount = -1;
  int bestAction = 0;
  for (int i = 0; i < n; ++i) {
    int c = 0;
    if (qir_runner_last_histogram_get(i, bits, sizeof(bits), &c) != 0) continue;
    int action = 0;
    for (int k = 0; k < 3 && bits[k]; ++k)
      if (bits[k] == '1') action |= (1 << k);
    if (c > bestCount) {
      bestCount = c;
      bestAction = action;
    }
  }
  return bestAction & 7;
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);
  int rank = 0, size = 1;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  const int iters = readIntArg(argc, argv, "iters", 3);
  const int nx = readIntArg(argc, argv, "nx", 128);
  const int ny = readIntArg(argc, argv, "ny", 128);
  const double residScale = 32.0;

  __quantum__rt__initialize(argc, reinterpret_cast<int8_t**>(argv));

  if (rank == 0) {
    std::cout << "[mpi-policy] ranks=" << size << " nx=" << nx << " ny=" << ny << " iters=" << iters
              << " (rank 0 executes quantum, histogram broadcast)\n";
  }

  int subdomains = 2;
  int innerIters = 8;
  int omegaIdx = 1;
  const double omegaChoices[3] = {0.6, 0.8, 1.0};
  int feedbackSeed = 1;

  for (int t = 0; t < iters; ++t) {
    double localResid = 0.0;
    const double omega = omegaChoices[omegaIdx % 3];
    const uint64_t hpcChk =
        hpc_stencil2d_step(nx, ny, subdomains, innerIters, feedbackSeed + rank, omega, &localResid);

    double globalResid = 0.0;
    MPI_Allreduce(&localResid, &globalResid, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

    int policyId = (int)std::floor(globalResid / residScale);
    if (policyId < 0) policyId = 0;
    if (policyId > 7) policyId = 7;

    runPolicyKernel(policyId);

    const int action = histogramArgmaxAction3bit();
    omegaIdx = action % 3;
    innerIters = 8 + ((action >> 1) & 1) * 4;
    subdomains = 2 + (action & 3);

    if (rank == 0) {
      std::cout << "[iter " << t << "] globalResid=" << globalResid << " policy=" << policyId
                << " action=" << action << " hpc=0x" << std::hex << hpcChk << std::dec
                << " next:omegaIdx=" << omegaIdx << " inner=" << innerIters << " sd=" << subdomains
                << "\n";
    }
  }

  __quantum__rt__finalize();
  MPI_Finalize();
  return 0;
}
