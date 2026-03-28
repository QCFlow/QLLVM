
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