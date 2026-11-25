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
 * Modified November 2026: lookahead/decay scoring, seeded trials and local tabu.
 * SabreSwap implementation
 *******************************************************************************/

#include "qllvm/passes/SabreSwap.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <set>

namespace qllvm {
namespace sabre {

SabreSwap::SabreSwap(const CouplingMap& coupling, const SabreOptions& opts)
    : coupling_(coupling), opts_(opts) {}

SabreSwapResult SabreSwap::run(const Circuit& circ,
                                const std::vector<int>& initialLayout) {
  SabreSwapResult best;
  size_t bestCount = std::numeric_limits<size_t>::max();
  for (int trial = 0; trial < std::max(1, opts_.swapTrials); ++trial) {
    auto candidate = runTrial(circ, initialLayout,
        (opts_.seed ? *opts_.seed : 42) + 104729u * trial);
    bool complete = std::all_of(candidate.layoutBeforeGate.begin(),
        candidate.layoutBeforeGate.end(), [](const auto& v) { return !v.empty(); });
    if (complete && candidate.insertions.size() < bestCount) {
      bestCount = candidate.insertions.size();
      best = std::move(candidate);
    }
  }
  if (bestCount == std::numeric_limits<size_t>::max())
    return runTrial(circ, initialLayout, opts_.seed ? *opts_.seed : 42);
  return best;
}

SabreSwapResult SabreSwap::runTrial(const Circuit& circ,
                                     const std::vector<int>& initialLayout,
                                     unsigned seed) {
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

  std::mt19937 rng(seed);
  std::vector<double> decay(circ.numQubits, 1.0);
  size_t swapsSinceProgress = 0;

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
    if (routed) {
      swapIterations = 0;
      swapsSinceProgress = 0;
      seenLayouts.clear();
      seenLayouts.insert(layout);
      std::fill(decay.begin(), decay.end(), 1.0);
      continue;
    }

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

    // Look ahead through dependency layers, skipping one-qubit gates.
    // Routing preserves the original QIR call order; future layers influence
    // the score without changing the runtime schedule or SSA dominance.
    std::vector<int> extended;
    std::vector<int> degree = inDegree;
    std::vector<bool> visited = executed;
    std::vector<int> layer = frontLayer;
    while (!layer.empty() && extended.size() < 20) {
      std::vector<int> next;
      for (int gi : layer) {
        visited[gi] = true;
        for (int successor : circ.successors[gi]) {
          if (--degree[successor] == 0 && !visited[successor])
            next.push_back(successor);
        }
      }
      for (int gi : next)
        if (circ.gates[gi].qubits.size() >= 2 && extended.size() < 20)
          extended.push_back(gi);
      layer = std::move(next);
    }
    auto distanceCost = [&](int a, int b, const std::vector<int>& gates) {
      if (gates.empty()) return 0.0;
      double total = 0;
      for (int gi : gates) {
        const Gate& gate = circ.gates[gi];
        auto mapped = [&](int v) {
          return layout[v == a ? b : v == b ? a : v];
        };
        int distance = coupling_.distance(mapped(gate.qubits[0]),
                                          mapped(gate.qubits[1]));
        if (distance < 0) return std::numeric_limits<double>::max();
        total += distance;
      }
      return total / gates.size();
    };
    double bestScore = std::numeric_limits<double>::max();
    std::vector<std::pair<int, int>> bestCandidates;
    // A bounded escape chooses a shortest-path step for the first blocked
    // interaction. It guarantees eventual progress on a connected graph.
    bool escape = swapsSinceProgress >= 5 * (size_t)circ.numQubits;
    for (const auto& sw : candidateSwaps) {
      std::vector<int> next = layout;
      std::swap(next[sw.first], next[sw.second]);
      if (!escape && seenLayouts.count(next)) continue;
      double score;
      if (escape) {
        const Gate& gate = circ.gates[minFrontGi];
        int before = coupling_.distance(layout[gate.qubits[0]], layout[gate.qubits[1]]);
        int after = coupling_.distance(next[gate.qubits[0]], next[gate.qubits[1]]);
        if (after >= before) continue;
        score = after;
      } else {
        score = distanceCost(sw.first, sw.second, twoQInFront);
        if (opts_.heuristic != "basic")
          score += 0.5 * distanceCost(sw.first, sw.second, extended);
        if (opts_.heuristic == "decay")
          score *= std::max(decay[sw.first], decay[sw.second]);
      }
      if (score < bestScore - 1e-10) {
        bestScore = score;
        bestCandidates = {sw};
      } else if (std::abs(score - bestScore) <= 1e-10) {
        bestCandidates.push_back(sw);
      }
    }
    if (bestCandidates.empty()) {
      seenLayouts.clear();
      swapsSinceProgress = 5 * (size_t)circ.numQubits;
      continue;
    }
    std::uniform_int_distribution<size_t> pick(0, bestCandidates.size()-1);
    auto chosen = bestCandidates[pick(rng)];
    int v0 = chosen.first, v1 = chosen.second;
    ++swapsSinceProgress;
    if (swapsSinceProgress % 5 == 0) std::fill(decay.begin(), decay.end(), 1.0);
    decay[v0] += 0.001;
    decay[v1] += 0.001;

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
