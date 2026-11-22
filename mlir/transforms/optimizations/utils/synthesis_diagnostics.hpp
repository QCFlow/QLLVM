// (C) Copyright QCFlow 2026. Licensed under Apache-2.0.
// Modified 2026-11-21: expensive synthesis diagnostics are explicitly opt-in.
#pragma once
#include <cstdlib>
#include <cstring>

namespace qllvm {
namespace synthesis {
inline bool diagnosticsEnabled() {
  static const bool enabled = [] {
    const char *value = std::getenv("QLLVM_SYNTHESIS_DIAGNOSTICS");
    return value && std::strcmp(value, "1") == 0;
  }();
  return enabled;
}
// Keep failed synthesis on huge inputs from producing millions of lines.
// No diagnostic decision changes the synthesis result or its safety checks.
inline bool allowDiagnostic() {
  if (!diagnosticsEnabled()) return false;
  static thread_local unsigned emitted = 0;
  return emitted++ < 256;
}
} // namespace synthesis
} // namespace qllvm
