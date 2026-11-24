// (C) Copyright QCFlow 2026. Licensed under Apache-2.0.
// Modified 2026-11-24: bounded, pure synthesis results survive optimization
// rounds within one compilation; no MLIR operation or SSA value is cached.
#pragma once
#include "gate_matrix.hpp"
#include "kak.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>

namespace qllvm {
namespace synthesis {
inline size_t storageBytes(const std::vector<utils::qop_t> &gates) {
  size_t bytes = gates.size() * sizeof(utils::qop_t);
  for (const auto &g : gates) bytes += g.first.size() + g.second.size() * sizeof(double);
  return bytes;
}
inline size_t storageBytes(const kak::KAK::SynthesisResult &result) {
  size_t bytes = sizeof(result);
  if (result) for (const auto &g : *result)
    bytes += sizeof(g) + g.name.size() + g.params.size() * sizeof(double) + g.indexs.size() * sizeof(int);
  return bytes;
}
template <typename Value> struct Memo {
  std::unordered_map<std::string, Value> values;
  size_t bytes = 0, hits = 0, misses = 0;
  bool find(const std::string &key, Value &out) {
    auto it = values.find(key);
    if (it == values.end()) { ++misses; return false; }
    ++hits; out = it->second; return true;
  }
  void insert(const std::string &key, const Value &value) {
    const size_t charge = key.size() + storageBytes(value) + sizeof(Value) + 64;
    if (values.size() < 4096 && key.size() <= 4096 && charge <= 4 * 1024 * 1024 - bytes) {
      if (values.emplace(key, value).second) bytes += charge;
    }
  }
};
struct CompilationCache {
  Memo<std::vector<utils::qop_t>> matrices1q, sequences1q;
  Memo<kak::KAK::SynthesisResult> matrices2q;
};
inline thread_local CompilationCache *currentCache = nullptr;
struct CacheScope {
  CompilationCache cache;
  CompilationCache *previous;
  CacheScope() : previous(currentCache) {
    const char *disabled = std::getenv("QLLVM_DISABLE_SYNTHESIS_CACHE");
    currentCache = disabled && std::strcmp(disabled, "1") == 0 ? nullptr : &cache;
  }
  ~CacheScope() { currentCache = previous; }
  CacheScope(const CacheScope &) = delete;
  CacheScope &operator=(const CacheScope &) = delete;
};
inline std::string basisKey(const std::unordered_set<std::string> &basis) {
  std::vector<std::string> names(basis.begin(), basis.end());
  std::sort(names.begin(), names.end());
  std::string result;
  for (const auto &name : names) { result += name; result.push_back('\0'); }
  result.push_back('\0');
  return result;
}
} // namespace synthesis
} // namespace qllvm
