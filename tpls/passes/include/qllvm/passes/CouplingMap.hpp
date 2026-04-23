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
 * Coupling map: hardware topology and all-pairs shortest path
 *******************************************************************************/
#pragma once

#include <utility>
#include <vector>

namespace qllvm {
namespace sabre {

class CouplingMap {
public:
  explicit CouplingMap(const std::vector<std::pair<int, int>>& edges);
  int numQubits() const { return num_qubits_; }
  bool adjacent(int p, int q) const;
  int distance(int p, int q) const;
  const std::vector<std::pair<int, int>>& edges() const { return edges_; }
  /// Neighbors of physical qubit p (O(deg) iteration, not O(E))
  const std::vector<int>& neighbors(int p) const;

private:
  std::vector<std::pair<int, int>> edges_;
  int num_qubits_ = 0;
  std::vector<std::vector<int>> adj_;
  std::vector<std::vector<int>> dist_;
  void computeDistanceMatrix();
};

}  // namespace sabre
}  // namespace qllvm
