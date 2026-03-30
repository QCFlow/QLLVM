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
 * SabreLayout: initial layout selection via bidirectional routing
 *******************************************************************************/
#pragma once

#include "qllvm/passes/CircuitExtractor.hpp"
#include "qllvm/passes/CouplingMap.hpp"
#include "qllvm/passes/SabreOptions.hpp"
#include <vector>

namespace qllvm {
namespace sabre {

class SabreLayout {
public:
  SabreLayout(const CouplingMap& coupling, const SabreOptions& opts);
  std::vector<int> run(const Circuit& circ);

private:
  const CouplingMap& coupling_;
  SabreOptions opts_;
  Circuit reverseCircuit(const Circuit& circ);
};

}  // namespace sabre
}  // namespace qllvm
