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
 * SabreSwap implementation - basic heuristic
 *******************************************************************************/

#include "qllvm/passes/SabreSwap.hpp"

#include <algorithm>
#include <limits>
#include <random>
#include <set>

namespace qllvm {
namespace sabre {

SabreSwap::SabreSwap(const CouplingMap& coupling, const SabreOptions& opts)
    : coupling_(coupling), opts_(opts) {}

double SabreSwap::scoreSwap(int v0, int v1, const std::vector<int>& layout,
                            const std::vector<int>& frontLayerGates,
                            const Circuit& circ) {
  std::vector<int> newLayout = layout;
  std::swap(newLayout[v0], newLayout[v1]);

  double cost = 0;
  for (int gi : frontLayerGates) {
    const Gate& g = circ.gates[gi];
    if (g.qubits.size() < 2) continue;
    int p0 = newLayout[g.qubits[0]];
    int p1 = newLayout[g.qubits[1]];
    int d = coupling_.distance(p0, p1);
    if (d < 0) return std::numeric_limits<double>::max();
    cost += d;
  }
  return cost;
}

SabreSwapResult SabreSwap::run(const Circuit& circ,
                                const std::vector<int>& initialLayout) {
  SabreSwapResult result;
  result.finalLayout = initialLayout;
  result.layoutBeforeGate.assign(circ.gates.size(), {});
  if (circ.numQubits == 0 || circ.gates.empty()) return result;

  std::vector<int> layout = initialLayout;
  std::vector<bool> executed(circ.gates.size(), false);
  std::vector<int> inDegree(circ.gates.size(), 0);
  for (size_t i = 0; i < circ.gates.size(); i++)
    inDegree[i] = 0;
  for (size_t i = 0; i < circ.successors.size(); i++)
    for (int s : circ.successors[i]) inDegree[s]++;

  std::vector<int> currentValueIndex(circ.numQubits, -1);

  std::mt19937 rng(opts_.seed ? *opts_.seed : 42);

  size_t executedCount = 0;
  size_t num2qGates = 0;
  for (const auto& g : circ.gates)
    if (g.qubits.size() >= 2) num2qGates++;

  // Limits must scale with circuit size: padded logical width (ancilla wires)
  // and dense CX layers can require many swap steps; tiny caps (e.g. 500)
  // abort routing on MQTBench-style circuits and trigger SabrePass fatal.
  const size_t kMaxSwapIterations =
      std::max<size_t>(2000, 80 * circ.gates.size() +
                                  32 * (size_t)std::max(0, circ.numQubits));
  const size_t kMaxInsertions =
      std::max<size_t>(500, 8 * num2qGates + 6 * circ.gates.size());
  size_t swapIterations = 0;
  std::set<std::vector<int>> seenLayouts;
  seenLayouts.insert(layout);

  while (executedCount < circ.gates.size()) {
    if (swapIterations++ >= kMaxSwapIterations) break;
    if (result.insertions.size() >= kMaxInsertions) break;
    std::vector<int> frontLayer;
    for (size_t i = 0; i < circ.gates.size(); i++) {
      if (executed[i] || inDegree[i] > 0) continue;
      frontLayer.push_back(i);
    }

    // Match LLVM/QIR block order: only the earliest ready gate may execute.
    // Otherwise we could "run" a later gate in simulation before swaps that
    // are inserted before an earlier blocking 2q gate, breaking layout replay in
    // applyPhysicalLayoutQir (and unitary equivalence).
    int minFrontGi =
        frontLayer.empty() ? -1
                           : *std::min_element(frontLayer.begin(), frontLayer.end());

    bool routed = false;
    for (int gi : frontLayer) {
      if (gi != minFrontGi) continue;
      const Gate& g = circ.gates[gi];
      if (g.qubits.size() < 2) {
        result.layoutBeforeGate[gi] = layout;
        executed[gi] = true;
        executedCount++;
        for (int q : g.qubits) currentValueIndex[q] = gi;
        for (int s : circ.successors[gi]) inDegree[s]--;
        routed = true;
        break;
      }
      int p0 = layout[g.qubits[0]];
      int p1 = layout[g.qubits[1]];
      if (coupling_.adjacent(p0, p1)) {
        result.layoutBeforeGate[gi] = layout;
        executed[gi] = true;
        executedCount++;
        for (int q : g.qubits) currentValueIndex[q] = gi;
        for (int s : circ.successors[gi]) inDegree[s]--;
        routed = true;
        break;
      }
    }
    if (routed) { swapIterations = 0; continue; }

    std::vector<int> twoQInFront;
    for (int gi : frontLayer) {
      if (circ.gates[gi].qubits.size() >= 2) twoQInFront.push_back(gi);
    }
    if (twoQInFront.empty()) continue;

    // invLayout[phys] = logical qubit index (or -1 if unused) - O(1) lookup
    std::vector<int> invLayout(coupling_.numQubits(), -1);
    for (int v = 0; v < circ.numQubits; v++) {
      int p = layout[v];
      if (p >= 0 && p < coupling_.numQubits()) invLayout[p] = v;
    }

    std::set<std::pair<int, int>> candidateSwaps;
    for (int gi : twoQInFront) {
      const Gate& g = circ.gates[gi];
      int v0 = g.qubits[0], v1 = g.qubits[1];
      int p0 = layout[v0], p1 = layout[v1];
      for (int other : coupling_.neighbors(p0)) {
        int vOther = invLayout[other];
        if (vOther >= 0)
          candidateSwaps.insert({std::min(v0, vOther), std::max(v0, vOther)});
      }
      for (int other : coupling_.neighbors(p1)) {
        int vOther = invLayout[other];
        if (vOther >= 0)
          candidateSwaps.insert({std::min(v1, vOther), std::max(v1, vOther)});
      }
    }

    if (candidateSwaps.empty()) break;

    double bestScore = std::numeric_limits<double>::max();
    std::vector<std::pair<int, int>> bestCandidates;
    std::vector<std::pair<std::pair<int, int>, double>> allScored;
    for (const auto& sw : candidateSwaps) {
      double sc = scoreSwap(sw.first, sw.second, layout, twoQInFront, circ);
      allScored.push_back({sw, sc});
      if (sc < bestScore) {
        bestScore = sc;
        bestCandidates = {{sw.first, sw.second}};
      } else if (sc == bestScore) {
        bestCandidates.push_back({sw.first, sw.second});
      }
    }

    auto applySwap = [&layout](int a, int b) {
      std::vector<int> next = layout;
      std::swap(next[a], next[b]);
      return next;
    };

    int v0 = -1, v1 = -1;
    std::vector<int> nextLayout;

    for (const auto& pick : bestCandidates) {
      nextLayout = applySwap(pick.first, pick.second);
      if (seenLayouts.find(nextLayout) == seenLayouts.end()) {
        v0 = pick.first;
        v1 = pick.second;
        break;
      }
    }

    if (v0 < 0) {
      std::sort(allScored.begin(), allScored.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; });
      for (const auto& [sw, sc] : allScored) {
        if (sc > bestScore * 1.5) break;
        nextLayout = applySwap(sw.first, sw.second);
        if (seenLayouts.find(nextLayout) == seenLayouts.end()) {
          v0 = sw.first;
          v1 = sw.second;
          break;
        }
      }
    }

    // Full-layout tabu can strand: all top swaps may revisit prior permutations
    // while routing is still required. Take a random feasible swap to explore.
    if (v0 < 0) {
      std::vector<std::pair<int, int>> finiteSwaps;
      finiteSwaps.reserve(allScored.size());
      for (const auto& [sw, sc] : allScored) {
        if (sc < std::numeric_limits<double>::max()) finiteSwaps.push_back(sw);
      }
      if (!finiteSwaps.empty()) {
        std::uniform_int_distribution<size_t> pick(0, finiteSwaps.size() - 1);
        auto sw = finiteSwaps[pick(rng)];
        v0 = sw.first;
        v1 = sw.second;
      }
    }

    // scoreSwap uses summed distances on the front layer; a swap can be needed
    // even when every candidate scores +∞ (e.g. lookahead pessimistic on large
    // maps). Any connected SWAP along the blocking CX qubits may still make
    // progress toward adjacency — pick one at random when no finite score exists.
    if (v0 < 0 && !candidateSwaps.empty()) {
      std::vector<std::pair<int, int>> allCand(candidateSwaps.begin(),
                                               candidateSwaps.end());
      std::uniform_int_distribution<size_t> pick(0, allCand.size() - 1);
      auto sw = allCand[pick(rng)];
      v0 = sw.first;
      v1 = sw.second;
    }

    if (v0 < 0) break;

    std::vector<int> layoutBeforeThisSwap = layout;
    std::swap(layout[v0], layout[v1]);
    seenLayouts.insert(layout);

    int insertBefore = minFrontGi;
    SwapInsertion ins;
    ins.layoutBeforeSwap = std::move(layoutBeforeThisSwap);
    ins.beforeGateIndex = insertBefore;
    ins.v0 = v0;
    ins.v1 = v1;
    // Search gates before insertBefore first (Values must be defined at insertion)
    for (int gi = 0; gi < insertBefore; gi++) {
      const Gate& g = circ.gates[gi];
      for (size_t i = 0; i < g.qubits.size() && i < g.operands.size(); i++) {
        if (g.qubits[i] == v0 && g.operands[i]) ins.val0 = g.operands[i];
        if (g.qubits[i] == v1 && g.operands[i]) ins.val1 = g.operands[i];
      }
    }
    // Include blocking gates (their operands are already computed before them)
    for (int gi : twoQInFront) {
      const Gate& g = circ.gates[gi];
      for (size_t i = 0; i < g.qubits.size() && i < g.operands.size(); i++) {
        if (g.qubits[i] == v0 && g.operands[i]) ins.val0 = g.operands[i];
        if (g.qubits[i] == v1 && g.operands[i]) ins.val1 = g.operands[i];
      }
    }
    // If still missing, search all gates (SabrePass will create load if needed)
    if (!ins.val0 || !ins.val1) {
      for (size_t gi = 0; gi < circ.gates.size(); gi++) {
        const Gate& g = circ.gates[gi];
        for (size_t i = 0; i < g.qubits.size() && i < g.operands.size(); i++) {
          if (g.qubits[i] == v0 && g.operands[i]) ins.val0 = g.operands[i];
          if (g.qubits[i] == v1 && g.operands[i]) ins.val1 = g.operands[i];
        }
        if (ins.val0 && ins.val1) break;
      }
    }

    result.insertions.push_back(ins);
  }

  result.finalLayout = layout;
  return result;
}

}  // namespace sabre
}  // namespace qllvm
