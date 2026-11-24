// (C) Copyright QCFlow 2026. Licensed under Apache-2.0.
// Modified 2026-11-24: rewriting passes report actual IR mutations to their
// enclosing optimization round; analysis-only work never reports a change.
#pragma once
namespace qllvm {
namespace optimization {
inline thread_local bool *currentChanged = nullptr;
inline void markChanged() {
  if (currentChanged) *currentChanged = true;
}
struct MutationScope {
  bool *previous;
  explicit MutationScope(bool &changed) : previous(currentChanged) {
    currentChanged = &changed;
  }
  ~MutationScope() {
    const bool changed = *currentChanged;
    currentChanged = previous;
    if (changed && previous) *previous = true;
  }
  MutationScope(const MutationScope &) = delete;
  MutationScope &operator=(const MutationScope &) = delete;
};
} // namespace optimization
} // namespace qllvm
