#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "qir-runner-runtime.h"

// Quantum kernel from qaoa.qasm (__internal_mlir_<stem>)
extern "C" {
void __quantum__rt__initialize(int argc, int8_t** argv);
void __quantum__rt__finalize();
void __internal_mlir_qaoa(void);

}

namespace {

// Path-3 MaxCut cost H_C = 0.5 <Z0 Z1> + 0.5 <Z1 Z2>; min eigenvalue -1 (optimal cut size 2).
// Histogram bitstring length 3: bits[0]=q0, bits[1]=q1, bits[2]=q2.
struct MaxCutStats {
  double z01;
  double z12;
  double energy;
  int shots;
};

MaxCutStats maxcut_expectations_from_hist_3q() {
  MaxCutStats out{};
  int n = qir_runner_last_histogram_size();
  for (int i = 0; i < n; ++i) {
    char bits[64];
    int c = 0;
    if (qir_runner_last_histogram_get(i, bits, sizeof(bits), &c) != 0)
      continue;
    if (std::strlen(bits) != 3)
      continue;
    auto ez = [](char b) { return b == '0' ? 1.0 : -1.0; };
    double e01 = ez(bits[0]) * ez(bits[1]);
    double e12 = ez(bits[1]) * ez(bits[2]);
    out.z01 += static_cast<double>(c) * e01;
    out.z12 += static_cast<double>(c) * e12;
    out.shots += c;
  }
  if (out.shots > 0) {
    double inv = 1.0 / static_cast<double>(out.shots);
    out.z01 *= inv;
    out.z12 *= inv;
    out.energy = 0.5 * out.z01 + 0.5 * out.z12;
  }
  return out;
}

}  // namespace

int main(int argc, char** argv) {
  __quantum__rt__initialize(argc, reinterpret_cast<int8_t**>(argv));

  std::cout << "Quantum kernel (3-qubit QAOA p=5 / path MaxCut) counts:\n";
  __internal_mlir_qaoa();

  std::cout << "Histogram:\n";
  int nh = qir_runner_last_histogram_size();
  for (int i = 0; i < nh; ++i) {
    char bits[64];
    int c = 0;
    if (qir_runner_last_histogram_get(i, bits, sizeof(bits), &c) == 0)
      std::cout << "  '" << bits << "': " << c << std::endl;
  }

  MaxCutStats s = maxcut_expectations_from_hist_3q();
  std::cout << "Shots used: " << s.shots << "\n";
  std::cout << "Est. <Z0 Z1> = " << s.z01 << "\n";
  std::cout << "Est. <Z1 Z2> = " << s.z12 << "\n";
  std::cout << "Est. <H_C>   = 0.5*<Z0Z1> + 0.5*<Z1Z2> ≈ " << s.energy
            << "  (ideal QAOA min = -1)\n";

  __quantum__rt__finalize();
  std::cout << "Done.\n";
  return 0;
}
