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
