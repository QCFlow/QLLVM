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
 * Modified November 2026: effective search controls and placement reporting.
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
  /// Optional JSON artifact describing the actual initial/final placement.
  std::string reportPath;
  std::string heuristic = "decay";
  int maxIterations = 3;
  int layoutTrials = 5;
  int swapTrials = 5;
  std::optional<unsigned> seed;
  /// After SABRE, rewrite QIR loads so gates use physical qubit indices.
  bool applyPhysicalLayoutToQir = true;

  bool enabled() const { return !couplingEdges.empty(); }
};

}  // namespace sabre
}  // namespace qllvm
