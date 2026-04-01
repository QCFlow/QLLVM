#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>
#include <vector>

#include "qir-runner-runtime.h"

/*
  felity.ipynb 234–343 的「边际概率 + GF(2) 恢复」流程，但**边际不再用稠密 Statevector
  直接算**，而是用 **qir-runner** 对 gmac_simon.qasm 执行后的**测量直方图**估计：

    P_hat(z_sub) ≈ count(z_sub) / shots

  再用 P_hat 构造 valid_z、加权采样、GF(2) 解 s' 与 H。
  统计误差随 shots 增大而减小（可用 ./exe -shots 8192）。

  若需与理想概率对比，可用 Qiskit Statevector 或其它工具离线算 reference。
*/

extern "C" {
void __quantum__rt__initialize(int argc, int8_t** argv);
void __quantum__rt__finalize();
void __internal_mlir_gmac_simon(void);
}

namespace {

constexpr int kN = 7;
constexpr int kW = 4;
constexpr int kSDiag = 51;
constexpr int kHSecret = 5;
constexpr int kAlpha0 = 1;
constexpr int kAlpha1 = 2;
constexpr int kEntangledBits[kW] = {0, 1, 4, 5};

constexpr int kIrred7 = 0b10000011;
constexpr int kN1 = kN + 1;
constexpr int kMarginalSlots = 1 << (kW + 1);

int gf_mul(int a, int b) {
  int r = 0;
  for (int i = 0; i < kN; ++i) {
    if ((b >> i) & 1)
      r ^= a;
    a <<= 1;
    if ((a >> kN) & 1)
      a ^= kIrred7;
  }
  return r & ((1 << kN) - 1);
}

int gf_inv(int a) {
  int r = 1;
  int base = a;
  int exp = (1 << kN) - 2;
  while (exp > 0) {
    if (exp & 1)
      r = gf_mul(r, base);
    base = gf_mul(base, base);
    exp >>= 1;
  }
  return r;
}

int gf_sqrt(int a) {
  int r = a;
  for (int i = 0; i < kN - 1; ++i)
    r = gf_mul(r, r);
  return r;
}

void print_config() {
  const int h2 = gf_mul(kHSecret, kHSecret);
  const int delta = kAlpha0 ^ kAlpha1;
  const int s_chk = gf_mul(delta, h2);
  std::cout << "=== GMAC Simon n=" << kN << ", w=" << kW << " ===\n";
  std::cout << "  H_secret=" << kHSecret << "  H^2=" << h2
            << "  delta=" << delta << "  delta*H^2=" << s_chk
            << (s_chk == kSDiag ? " (== s')\n" : " (!!= s' CONFIG)\n");
  std::cout << "  s'=" << kSDiag << "  entangled_bits: ";
  for (int b : kEntangledBits)
    std::cout << b << ' ';
  std::cout << "\n";
}

int lowest_bit_idx(unsigned v) {
  assert(v != 0);
  int k = 0;
  while (((v >> k) & 1u) == 0)
    ++k;
  return k;
}

unsigned s_sub_mask() {
  unsigned s = 1u;
  for (int k = 0; k < kW; ++k) {
    const int bp = kEntangledBits[k];
    const unsigned bit = ((kSDiag >> bp) & 1) ? 1u : 0u;
    s |= (bit << static_cast<unsigned>(k + 1));
  }
  return s;
}

std::vector<int> independent_bits() {
  std::vector<int> ind;
  for (int i = 0; i < kN; ++i) {
    bool ent = false;
    for (int b : kEntangledBits) {
      if (b == i)
        ent = true;
    }
    if (!ent)
      ind.push_back(i);
  }
  return ind;
}

// bitstr: 长度 kW+1，bits[j] 为 c[j]（b, x_ent0…），与 gmac_simon.qasm measure 顺序一致
bool parse_zsub_key(const char* bitstr, unsigned& key_out) {
  if (std::strlen(bitstr) != static_cast<size_t>(kW + 1))
    return false;
  key_out = 0;
  for (int j = 0; j <= kW; ++j) {
    if (bitstr[j] == '1')
      key_out |= (1u << j);
    else if (bitstr[j] != '0')
      return false;
  }
  return true;
}

int parity_u(unsigned x) {
  int p = 0;
  while (x) {
    p ^= 1;
    x &= x - 1;
  }
  return p;
}

unsigned z_sub_from_key(int key) { return static_cast<unsigned>(key & ((1 << (kW + 1)) - 1)); }

unsigned z_global_from_z_sub(unsigned z_sub_samp, std::mt19937& gen) {
  const auto ind = independent_bits();
  unsigned zg = z_sub_samp & 1u;
  for (int k = 0; k < kW; ++k) {
    const int bp = kEntangledBits[k];
    zg |= (((z_sub_samp >> (1 + k)) & 1u) << static_cast<unsigned>(1 + bp));
  }
  std::uniform_int_distribution<int> bit01(0, 1);
  for (int bp : ind)
    zg |= (static_cast<unsigned>(bit01(gen)) << static_cast<unsigned>(1 + bp));
  return zg;
}

struct GF2Solver {
  std::vector<unsigned> rows;
  std::vector<int> pivots;
  int rank = 0;

  bool add(unsigned v) {
    unsigned r = v;
    for (int i = 0; i < rank; ++i) {
      if ((r >> pivots[static_cast<size_t>(i)]) & 1u)
        r ^= rows[static_cast<size_t>(i)];
    }
    if (r == 0)
      return false;
    const int p = lowest_bit_idx(r);
    rows.push_back(r);
    pivots.push_back(p);
    ++rank;
    return true;
  }

  unsigned solve(int nb) {
    std::vector<bool> is_piv(static_cast<size_t>(nb), false);
    for (int p : pivots)
      is_piv[static_cast<size_t>(p)] = true;
    int free_col = -1;
    for (int c = 0; c < nb; ++c) {
      if (!is_piv[static_cast<size_t>(c)]) {
        free_col = c;
        break;
      }
    }
    if (free_col < 0)
      return 0;
    unsigned s = 1u << free_col;
    for (int i = rank - 1; i >= 0; --i) {
      const int col = pivots[static_cast<size_t>(i)];
      const unsigned row = rows[static_cast<size_t>(i)];
      int par = 0;
      for (int c = col + 1; c < nb; ++c) {
        if (((row >> c) & 1u) && ((s >> c) & 1u))
          par ^= 1;
      }
      if (par)
        s |= (1u << col);
    }
    return s;
  }
};

/** 由 qir-runner 最后一次直方图填 marginal_est[key]=P_hat，返回 total_shots。 */
int marginal_from_histogram(std::vector<double>& marginal_est) {
  std::vector<int> counts(static_cast<size_t>(kMarginalSlots), 0);
  int total = 0;
  int nh = qir_runner_last_histogram_size();
  for (int i = 0; i < nh; ++i) {
    char bitstr[64];
    int c = 0;
    if (qir_runner_last_histogram_get(i, bitstr, sizeof(bitstr), &c) != 0)
      continue;
    unsigned key = 0;
    if (!parse_zsub_key(bitstr, key))
      continue;
    counts[key] += c;
    total += c;
  }
  marginal_est.assign(static_cast<size_t>(kMarginalSlots), 0.0);
  if (total <= 0)
    return 0;
  const double inv = 1.0 / static_cast<double>(total);
  for (size_t k = 0; k < counts.size(); ++k)
    marginal_est[k] = static_cast<double>(counts[k]) * inv;
  return total;
}

}  // namespace

int main(int argc, char** argv) {
  print_config();

  const unsigned s_sub = s_sub_mask();
  const int delta = kAlpha0 ^ kAlpha1;
  const int delta_inv = gf_inv(delta);

  __quantum__rt__initialize(argc, reinterpret_cast<int8_t**>(argv));
  std::cout << "\n--- qir-runner (gmac_simon.qasm) ---\n";
  __internal_mlir_gmac_simon();

  std::cout << "\nRaw histogram:\n";
  int nh = qir_runner_last_histogram_size();
  for (int i = 0; i < nh; ++i) {
    char bitstr[64];
    int c = 0;
    if (qir_runner_last_histogram_get(i, bitstr, sizeof(bitstr), &c) == 0)
      std::cout << "  '" << bitstr << "': " << c << "\n";
  }

  std::vector<double> marginal;
  const int total_shots = marginal_from_histogram(marginal);
  if (total_shots <= 0) {
    std::cerr << "No valid shots / histogram empty.\n";
    __quantum__rt__finalize();
    return 1;
  }

  std::cout << "\n--- Estimated marginal P_hat = count/shots (shots=" << total_shots << ") ---\n";
  double est_frac_z_dot_zero = 0.0;
  std::cout << "  z·s_sub 检验 (s_sub=" << s_sub << "):\n";
  std::vector<std::pair<unsigned, double>> valid_z;
  for (int key = 0; key < kMarginalSlots; ++key) {
    const double prob = marginal[static_cast<size_t>(key)];
    if (prob <= 0.0)
      continue;
    const unsigned zv = z_sub_from_key(key);
    const int dot = parity_u(zv & s_sub);
    est_frac_z_dot_zero += (dot == 0) ? prob : 0.0;
    if (prob > 0.001) {
      std::cout << "    z_sub=" << zv << "  P_hat=" << prob << "  z·s_sub%2=" << dot
                << (dot == 0 ? "  ok\n" : "  !\n");
    }
    if (prob > 0.01 && zv != 0)
      valid_z.emplace_back(zv, prob);
  }
  std::cout << "  E_hat[ 1{z·s_sub=0} ] ≈ " << est_frac_z_dot_zero << "\n";

  if (valid_z.empty()) {
    for (int key = 0; key < kMarginalSlots; ++key) {
      const double prob = marginal[static_cast<size_t>(key)];
      const unsigned zv = z_sub_from_key(key);
      if (prob > 0.0 && zv != 0)
        valid_z.emplace_back(zv, prob);
    }
    double s = 0.0;
    for (const auto& pr : valid_z)
      s += pr.second;
    for (auto& pr : valid_z)
      pr.second /= s;
    std::cout << "  (relaxed valid_z: all observed z_sub!=0, renormalized)\n";
  }

  double psum = 0.0;
  for (const auto& pr : valid_z)
    psum += pr.second;
  std::vector<double> weights;
  std::vector<unsigned> z_vals;
  for (const auto& pr : valid_z) {
    z_vals.push_back(pr.first);
    weights.push_back(pr.second / psum);
  }
  if (z_vals.empty()) {
    std::cerr << "valid_z still empty.\n";
    __quantum__rt__finalize();
    return 1;
  }

  std::cout << "\n--- GF(2) recovery (sampling from P_hat) ---\n";
  std::mt19937 gen(42);
  std::discrete_distribution<int> pick(weights.begin(), weights.end());

  GF2Solver solver;
  int trial = 0;
  while (solver.rank < kN1 - 1) {
    ++trial;
    const unsigned z_sub_samp = z_vals[static_cast<size_t>(pick(gen))];
    unsigned z_global = z_global_from_z_sub(z_sub_samp, gen);
    if (z_global == 0)
      continue;
    solver.add(z_global);
  }

  const unsigned s_solved = solver.solve(kN1);
  const int s_prime_recovered = static_cast<int>((s_solved >> 1) & ((1u << kN) - 1));
  const int h2_rec = gf_mul(s_prime_recovered, delta_inv);
  const int h_rec = gf_sqrt(h2_rec);

  std::cout << "  loop trials: " << trial << "\n";
  std::cout << "  s' recovered = " << s_prime_recovered
            << (s_prime_recovered == kSDiag ? "  ok\n" : "  (noisy / wrong)\n");
  std::cout << "  H recovered  = " << h_rec << " (expect " << kHSecret << ")"
            << (h_rec == kHSecret ? "  ok\n" : "  (noisy / wrong)\n");

  __quantum__rt__finalize();
  std::cout << "Done.\n";
  return 0;
}
