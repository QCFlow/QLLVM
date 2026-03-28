/*******************************************************************************
 * CouplingMap implementation - BFS for distance matrix
 *******************************************************************************/

#include "qllvm/passes/CouplingMap.hpp"

#include <algorithm>
#include <queue>
#include <set>

namespace qllvm {
namespace sabre {

CouplingMap::CouplingMap(const std::vector<std::pair<int, int>>& edges) {
  edges_ = edges;
  std::set<int> qubits;
  for (const auto& e : edges_) {
    qubits.insert(e.first);
    qubits.insert(e.second);
  }
  num_qubits_ = qubits.empty() ? 0 : (*qubits.rbegin()) + 1;
  computeDistanceMatrix();
}

bool CouplingMap::adjacent(int p, int q) const {
  if (p < 0 || p >= num_qubits_ || q < 0 || q >= num_qubits_) return false;
  for (int n : adj_[p]) {
    if (n == q) return true;
  }
  return false;
}

const std::vector<int>& CouplingMap::neighbors(int p) const {
  static const std::vector<int> empty;
  if (p < 0 || p >= num_qubits_) return empty;
  return adj_[p];
}

int CouplingMap::distance(int p, int q) const {
  if (p < 0 || p >= num_qubits_ || q < 0 || q >= num_qubits_) return -1;
  return dist_[p][q];
}

void CouplingMap::computeDistanceMatrix() {
  adj_.assign(num_qubits_, std::vector<int>());
  for (const auto& e : edges_) {
    int a = e.first, b = e.second;
    if (a < num_qubits_ && b < num_qubits_) {
      adj_[a].push_back(b);
      adj_[b].push_back(a);
    }
  }

  dist_.assign(num_qubits_, std::vector<int>(num_qubits_, -1));
  for (int i = 0; i < num_qubits_; i++) dist_[i][i] = 0;

  for (int src = 0; src < num_qubits_; src++) {
    std::queue<int> q;
    q.push(src);
    while (!q.empty()) {
      int u = q.front();
      q.pop();
      for (int v : adj_[u]) {
        if (dist_[src][v] < 0) {
          dist_[src][v] = dist_[src][u] + 1;
          q.push(v);
        }
      }
    }
  }
}

}  // namespace sabre
}  // namespace qllvm
