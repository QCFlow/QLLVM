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
