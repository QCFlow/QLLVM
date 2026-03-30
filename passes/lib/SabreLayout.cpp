/*
 * This code is part of QLLVM.
 *
 * (C) Copyright QCFlow 2026.
 *
 * This code is licensed under the Apache License, Version 2.0. You may
 * obtain a copy of this license in the LICENSE file in the root directory
 * of this source tree or at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * Any modifications or derivative works of this code must retain this
 * copyright notice, and modified files need to carry a notice indicating
 * that they have been altered from the originals.
 */
/*******************************************************************************
 * SabreLayout - bidirectional layout selection
 *******************************************************************************/

#include "qllvm/passes/SabreLayout.hpp"
#include "qllvm/passes/SabreSwap.hpp"

#include <algorithm>
#include <limits>
#include <random>

namespace qllvm {
namespace sabre {

SabreLayout::SabreLayout(const CouplingMap& coupling, const SabreOptions& opts)
    : coupling_(coupling), opts_(opts) {}

Circuit SabreLayout::reverseCircuit(const Circuit& circ) {
  Circuit rev;
  rev.numQubits = circ.numQubits;
  rev.gates.resize(circ.gates.size());
  std::reverse_copy(circ.gates.begin(), circ.gates.end(), rev.gates.begin());
  rev.successors.assign(rev.gates.size(), {});
  // Original edge p -> g means gate g runs after p. Reversed pass should apply
  // later original gates first (smaller rev index). So rev index (n-1-g) must
  // precede (n-1-p): add edge (n-1-g) -> (n-1-p). The previous construction
  // flipped this and routed "backwards" circuits in forward order, corrupting
  // bidirectional layout and the final forward routing in SabrePass.
  const int n = (int)rev.gates.size();
  for (int p = 0; p < n; p++) {
    const int i_p = n - 1 - p;
    for (int g : circ.successors[p]) {
      if (g < 0 || g >= n) continue;
      const int i_g = n - 1 - g;
      rev.successors[i_g].push_back(i_p);
    }
  }
  return rev;
}

std::vector<int> SabreLayout::run(const Circuit& circ) {
  if (circ.numQubits == 0 || circ.gates.empty()) {
    std::vector<int> trivial(circ.numQubits);
    for (int i = 0; i < circ.numQubits; i++) trivial[i] = i;
    return trivial;
  }

  if (opts_.initialLayout && opts_.initialLayout->size() >= (size_t)circ.numQubits)
    return std::vector<int>(opts_.initialLayout->begin(),
                            opts_.initialLayout->begin() + circ.numQubits);

  int numPhys = coupling_.numQubits();
  if (circ.numQubits > numPhys) {
    std::vector<int> trivial(circ.numQubits);
    for (int i = 0; i < circ.numQubits; i++) trivial[i] = i % numPhys;
    return trivial;
  }

  std::mt19937 rng(opts_.seed ? *opts_.seed : 42);
  std::vector<int> bestLayout;
  size_t bestSwaps = std::numeric_limits<size_t>::max();

  Circuit revCirc = reverseCircuit(circ);
  SabreSwap swap(coupling_, opts_);

  std::vector<int> physOrder(numPhys);
  for (int i = 0; i < numPhys; i++) physOrder[i] = i;

  for (int trial = 0; trial < opts_.layoutTrials; trial++) {
    std::vector<int> layout(circ.numQubits);
    std::shuffle(physOrder.begin(), physOrder.end(), rng);
    for (int i = 0; i < circ.numQubits && i < numPhys; i++)
      layout[i] = physOrder[i];

    for (int iter = 0; iter < opts_.maxIterations; iter++) {
      auto fwdResult = swap.run(circ, layout);
      layout = fwdResult.finalLayout;

      auto revResult = swap.run(revCirc, layout);
      layout = revResult.finalLayout;
    }

    auto finalResult = swap.run(circ, layout);
    if (finalResult.insertions.size() < bestSwaps) {
      bestSwaps = finalResult.insertions.size();
      // `swap.run` does not mutate `layout`; it is still the initial mapping
      // for this scoring forward pass. SabrePass must receive that same
      // pre-routing layout so its `swap.run(circ, initialLayout)` matches the
      // trial we scored. `finalResult.finalLayout` is post-routing and wrong.
      bestLayout = layout;
    }
  }

  if (bestLayout.empty()) {
    std::vector<int> trivial(circ.numQubits);
    for (int i = 0; i < circ.numQubits; i++) trivial[i] = i;
    return trivial;
  }
  return bestLayout;
}

}  // namespace sabre
}  // namespace qllvm
