// (C) Copyright QCFlow 2026. Licensed under Apache-2.0.
// Modified 2026-11-21: exact, bounded snapshots for conservative no-change exit.
#pragma once
#include "mlir/IR/BuiltinOps.h"
#include <cstdint>
#include <vector>

namespace qllvm {
// Deliberately include operation/value identities and ALL attributes. A rewrite
// that recreates equivalent gates is considered changed. No gate-count heuristic,
// floating tolerance, omitted control-flow state, or probabilistic hash is used.
// Encoded pointers are never dereferenced after the snapshot is taken.
struct OptimizationRoundSnapshot {
  std::vector<uintptr_t> words;
  bool complete = true;
  static constexpr size_t maxWords = 16 * 1024 * 1024 / sizeof(uintptr_t);
  void add(uintptr_t value) {
    if (!complete) return;
    if (words.size() == maxWords) {
      complete = false;
      words.clear();
    } else words.push_back(value);
  }
  template<class T> void pointer(T *value) {
    add(reinterpret_cast<uintptr_t>(value));
  }
  explicit OptimizationRoundSnapshot(mlir::Operation *root) {
    root->walk([&](mlir::Operation *op) {
      if (!complete) return;
      pointer(op); pointer(op->getBlock());
      pointer(op->getName().getStringRef().data());
      pointer(op->getAttrDictionary().getAsOpaquePointer());
      add(op->getNumOperands());
      for (auto operand : op->getOperands()) pointer(operand.getAsOpaquePointer());
      add(op->getNumResults());
      for (auto result : op->getResults()) {
        pointer(result.getAsOpaquePointer());pointer(result.getType().getAsOpaquePointer());
      }
      add(op->getNumSuccessors());
      for (auto *successor : op->getSuccessors()) pointer(successor);
      add(op->getNumRegions());
      for (auto &region : op->getRegions()) {
        pointer(&region);add(region.getBlocks().size());
        for (auto &block : region) {
          pointer(&block);add(block.getNumArguments());
          for (auto argument : block.getArguments()) {
            pointer(argument.getAsOpaquePointer());pointer(argument.getType().getAsOpaquePointer());
          }
        }
      }
    });
  }
  bool unchanged(const OptimizationRoundSnapshot &other) const {
    return complete && other.complete && words == other.words;
  }
};
} // namespace qllvm
