#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "qir-runner-runtime.h"

// Quantum kernel from vqe.qasm (__internal_mlir_<stem>)
extern "C" {
void __quantum__rt__initialize(int argc, int8_t** argv);
void __quantum__rt__finalize();
void __internal_mlir_vqe(void);
}

namespace {

// Two-qubit computational-basis counts → Pauli expectations (same basis as Z⊗Z diag).
struct Pauli2 {
  double zz;
  double z0;
  double z1;
  int shots;
};

Pauli2 expectations_from_hist_2q() {
  Pauli2 out{};
  int n = qir_runner_last_histogram_size();
  for (int i = 0; i < n; ++i) {
    char bits[64];
    int c = 0;
    if (qir_runner_last_histogram_get(i, bits, sizeof(bits), &c) != 0)
      continue;
    if (std::strlen(bits) != 2)
      continue;
    int b0 = bits[0] == '0' ? 0 : 1;
    int b1 = bits[1] == '0' ? 0 : 1;
    double ez0 = (b0 == 0) ? 1.0 : -1.0;
    double ez1 = (b1 == 0) ? 1.0 : -1.0;
    out.z0 += static_cast<double>(c) * ez0;
    out.z1 += static_cast<double>(c) * ez1;
    out.zz += static_cast<double>(c) * ez0 * ez1;
    out.shots += c;
  }
  if (out.shots > 0) {
    double inv = 1.0 / static_cast<double>(out.shots);
    out.z0 *= inv;
    out.z1 *= inv;
    out.zz *= inv;
  }
  return out;
}

// Toy 2-qubit Hamiltonian: H = h0 ZI + h1 IZ + h2 ZZ (coefficients as in small spin models).
double toy_energy(const Pauli2& p, double h_zi, double h_iz, double h_zz) {
  return h_zi * p.z0 + h_iz * p.z1 + h_zz * p.zz;
}

}  // namespace

int main(int argc, char** argv) {
  __quantum__rt__initialize(argc, reinterpret_cast<int8_t**>(argv));

  std::cout << "Quantum kernel (2-qubit VQE-style ansatz) counts:\n";
  __internal_mlir_vqe();

  std::cout << "Histogram:\n";
  int nh = qir_runner_last_histogram_size();
  for (int i = 0; i < nh; ++i) {
    char bits[64];
    int c = 0;
    if (qir_runner_last_histogram_get(i, bits, sizeof(bits), &c) == 0)
      std::cout << "  '" << bits << "': " << c << std::endl;
  }

  Pauli2 p = expectations_from_hist_2q();
  std::cout << "Shots used: " << p.shots << "\n";
  std::cout << "Est. <Z I>  = " << p.z0 << "\n";
  std::cout << "Est. <I Z>  = " << p.z1 << "\n";
  std::cout << "Est. <Z Z>  = " << p.zz << "\n";

  const double h_zi = 0.25;
  const double h_iz = 0.25;
  const double h_zz = 0.5;
  double E = toy_energy(p, h_zi, h_iz, h_zz);
  std::cout << "Toy VQE energy <H> = " << h_zi << "*<ZI> + " << h_iz << "*<IZ> + " << h_zz
            << "*<ZZ> ≈ " << E << "\n";

  __quantum__rt__finalize();
  std::cout << "Done.\n";
  return 0;
}
