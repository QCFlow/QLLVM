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
 *
 * Modified by QCFlow (2026) for QLLVM project.
 */

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
