/*******************************************************************************
 * Backend registry for code generation backends
 *******************************************************************************/
#pragma once

#include "Backend.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace qllvm {

class BackendRegistry {
public:
  static BackendRegistry& instance();

  void registerBackend(std::unique_ptr<Backend> backend);
  Backend* get(const std::string& name) const;
  std::vector<std::string> listBackends() const;

private:
  BackendRegistry() = default;
  std::unordered_map<std::string, std::unique_ptr<Backend>> backends_;
};

}  // namespace qllvm
