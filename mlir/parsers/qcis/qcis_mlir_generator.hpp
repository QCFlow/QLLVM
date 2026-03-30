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

#pragma once

#include "Quantum/QuantumOps.h"
#include "mlir_generator.hpp"

namespace qllvm {
class qcis_visitor;

class qcisMLIRGenerator : public qllvm::QuantumMLIRGenerator {
 protected:
  std::shared_ptr<qcis_visitor> visitor;

 public:
  qcisMLIRGenerator(mlir::MLIRContext &context)
      : QuantumMLIRGenerator(context) {
    m_module = mlir::ModuleOp::create(builder.getUnknownLoc());
  }
  qcisMLIRGenerator(mlir::OpBuilder b, mlir::MLIRContext &ctx)
      : QuantumMLIRGenerator(b, ctx) {
    m_module = mlir::ModuleOp::create(builder.getUnknownLoc());
  }

  void initialize_mlirgen(const std::string func_name,
                          std::vector<mlir::Type> arg_types,
                          std::vector<std::string> arg_var_names,
                          std::vector<std::string> var_attributes,
                          mlir::Type return_type);
  void initialize_mlirgen(
      bool add_entry_point = true, const std::string file_name = "",
      std::map<std::string, std::string> extra_quantum_args = {}) override;
  void mlirgen(const std::string &src) override;
  void finalize_mlirgen() override;
};
} // namespace qllvm