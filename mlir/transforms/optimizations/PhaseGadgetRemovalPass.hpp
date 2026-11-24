// (C) Copyright QCFlow 2026. Licensed under Apache-2.0.
// Modified 2026-11-24: charge a closed parity-phase gadget once, before
// lowering obscures its two-CX cancellation opportunity. Never loosen the
// global near-identity deletion budget or silently approximate controlled IR.
#pragma once
#include "Quantum/QuantumOps.h"
#include "utils/approximation_budget.hpp"
#include "utils/mutation_tracking.hpp"
#include "utils/op.hpp"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/DenseSet.h"
#include <algorithm>

namespace qllvm {
struct PhaseGadgetRemovalPass
    : public mlir::PassWrapper<PhaseGadgetRemovalPass, mlir::OperationPass<mlir::ModuleOp>> {
  void runOnOperation() final {
    using Gate = mlir::quantum::ValueSemanticsInstOp;
    struct Candidate { Gate first, phase, last; double angle, cost; };
    std::vector<Candidate> candidates;
    getOperation().walk([&](Gate first) {
      if (first->getParentOfType<mlir::quantum::CtrlURegion>() || first.name() != "cx" ||
          first.getNumResults() != 2 || !first.getResult(0).hasOneUse() ||
          !first.getResult(1).hasOneUse()) return;
      auto phase = mlir::dyn_cast<Gate>(*first.getResult(1).user_begin());
      auto last = mlir::dyn_cast<Gate>(*first.getResult(0).user_begin());
      if (!phase || !last || phase.getNumResults() != 1 || !phase.getResult(0).hasOneUse() ||
          last.name() != "cx" || last.getNumResults() != 2 ||
          phase->getBlock() != first->getBlock() || last->getBlock() != first->getBlock() ||
          last.getOperand(0) != first.getResult(0) ||
          last.getOperand(1) != phase.getResult(0) ||
          *phase.getResult(0).user_begin() != last.getOperation()) return;
      double angle;
      if ((phase.name() == "rz" || phase.name() == "p" || phase.name() == "u1") &&
          phase.getNumOperands() == 2) angle = OP::tryGetConstAngle(phase.getOperand(1));
      else if ((phase.name() == "u3" || phase.name() == "u") && phase.getNumOperands() == 4 &&
               OP::tryGetConstAngle(phase.getOperand(1)) == 0.0)
        angle = OP::tryGetConstAngle(phase.getOperand(2)) + OP::tryGetConstAngle(phase.getOperand(3));
      else return;
      if (!std::isfinite(angle)) return;
      const double cost = approximation::rotationDistanceBound(angle);
      // CX Rz(theta) CX is RZZ(theta). A phase gate differs only by a
      // global phase outside coherent control. Conjugation preserves the
      // channel diamond-distance bound used by the existing rotation budget.
      if (cost <= approximation::current.limit)
        candidates.push_back({first, phase, last, angle, cost});
    });
    std::stable_sort(candidates.begin(), candidates.end(), [](const auto &a, const auto &b) {
      return a.cost < b.cost; // Each accepted gadget removes two entanglers.
    });
    llvm::DenseSet<mlir::Operation *> removed;
    std::vector<Gate> dead;
    for (auto &c : candidates) {
      if (removed.count(c.first) || removed.count(c.phase) || removed.count(c.last)) continue;
      if (!approximation::chargeRotation(c.angle)) continue;
      c.last.getResult(0).replaceAllUsesWith(c.first.getOperand(0));
      c.last.getResult(1).replaceAllUsesWith(c.first.getOperand(1));
      for (auto gate : {c.first,c.phase,c.last}) {
        removed.insert(gate);dead.push_back(gate);
      }
    }
    for (auto gate : dead) gate->dropAllUses();
    for (auto gate : dead) { optimization::markChanged();gate.erase(); }
  }
};
} // namespace qllvm
