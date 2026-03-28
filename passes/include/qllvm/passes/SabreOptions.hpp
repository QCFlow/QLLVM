/*******************************************************************************
 * SABRE pass options - coupling map, heuristic, trial counts
 *******************************************************************************/
#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace qllvm {
namespace sabre {

struct SabreOptions {
  std::vector<std::pair<int, int>> couplingEdges;
  std::optional<std::vector<int>> initialLayout;
  std::string kernelName;
  std::string heuristic = "basic";
  int maxIterations = 3;
  int layoutTrials = 1;
  int swapTrials = 1;
  std::optional<unsigned> seed;
  /// After SABRE, rewrite QIR loads so gates use physical qubit indices.
  bool applyPhysicalLayoutToQir = true;

  bool enabled() const { return !couplingEdges.empty(); }
};

}  // namespace sabre
}  // namespace qllvm
