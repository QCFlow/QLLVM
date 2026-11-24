// (C) Copyright QCFlow 2026. Licensed under Apache-2.0.
// Modified 2026-11-24: deterministic worklist for exact local cleanup.
#pragma once
#include "Quantum/QuantumOps.h"
#include "utils/mutation_tracking.hpp"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/DenseSet.h"
#include <deque>

namespace qllvm {
struct LocalCleanupPass
    : public mlir::PassWrapper<LocalCleanupPass, mlir::OperationPass<mlir::ModuleOp>> {
  void runOnOperation() final {
    using Gate = mlir::quantum::ValueSemanticsInstOp;
    llvm::DenseSet<mlir::Operation *> live, queued;
    std::deque<mlir::Operation *> pending;
    getOperation().walk([&](Gate op) {
      live.insert(op.getOperation());
      queued.insert(op.getOperation());
      pending.push_back(op.getOperation());
    });
    auto enqueue = [&](mlir::Operation *op) {
      if (live.count(op) && queued.insert(op).second) pending.push_back(op);
    };
    auto inverse = [](llvm::StringRef first, llvm::StringRef second) {
      if (first == second) return first == "x" || first == "y" || first == "z" || first == "h";
      return (first == "s" && second == "sdg") || (first == "sdg" && second == "s") ||
             (first == "t" && second == "tdg") || (first == "tdg" && second == "t");
    };
    while (!pending.empty()) {
      auto *raw = pending.front();pending.pop_front();queued.erase(raw);
      if (!live.count(raw)) continue; // Erased queued handles are never read.
      auto first = mlir::cast<Gate>(raw);
      if (first.getNumResults() == 0 || !first.getResult(0).hasOneUse()) continue;
      auto second = mlir::dyn_cast<Gate>(*first.getResult(0).user_begin());
      if (!second || first->getBlock() != second->getBlock()) continue;
      bool reset = first.name() == "reset" && second.name() == "reset";
      bool cancel = false;
      if (first.getNumResults() == 1 && second.getNumResults() == 1)
        cancel = inverse(first.name(), second.name());
      else if (first.getNumResults() == 2 && second.getNumResults() == 2 &&
               first.name() == second.name() &&
               (first.name() == "cx" || first.name() == "cnot" || first.name() == "cz")) {
        cancel = first.getResult(1).hasOneUse() &&
                 *first.getResult(1).user_begin() == second.getOperation() &&
                 second.getOperand(0) == first.getResult(0) &&
                 second.getOperand(1) == first.getResult(1);
      }
      if (!reset && !cancel) continue;
      // Revisit only producers/users bordering the removed pair. All handles
      // are checked against live before dereferencing, even across long runs.
      std::vector<mlir::Operation *> neighbors;
      for (auto gate : {first, second}) {
        for (auto operand : gate.getOperands()) neighbors.push_back(operand.getDefiningOp());
        for (auto result : gate.getResults())
          for (auto *user : result.getUsers()) neighbors.push_back(user);
      }
      if (reset) second.getResult(0).replaceAllUsesWith(first.getResult(0));
      else for (size_t i = 0; i < first.getNumResults(); ++i)
        second.getResult(i).replaceAllUsesWith(first.getOperand(i));
      live.erase(second.getOperation());
      second.erase();
      if (!reset) { live.erase(first.getOperation()); first.erase(); }
      optimization::markChanged();
      for (auto *neighbor : neighbors) enqueue(neighbor);
    }
  }
};
} // namespace qllvm
