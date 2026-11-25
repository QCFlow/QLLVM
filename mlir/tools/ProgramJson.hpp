// Copyright QCFlow 2026. Licensed under Apache-2.0.
// Direct v0.1 ProgramBuilder protocol -> quantum MLIR. No assembly parser.
#pragma once
#include "mlir_generator.hpp"
#include "Quantum/QuantumOps.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/Error.h"
#include "mlir/Dialect/StandardOps/IR/Ops.h"
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>

namespace qllvm {
class ProgramJsonGenerator : public QuantumMLIRGenerator {
  mlir::FuncOp function;
  static void reject(const std::string &message) {
    throw std::runtime_error("[program-json] " + message);
  }
  static const llvm::json::Object &object(const llvm::json::Value &value) {
    auto *result = value.getAsObject();
    if (!result) reject("expected an object");
    return *result;
  }
  static void fields(const llvm::json::Object &value,
                     std::initializer_list<llvm::StringRef> names) {
    if (value.size() != names.size()) reject("unexpected or missing contract fields");
    for (auto name : names) if (!value.get(name)) reject("missing field " + name.str());
  }
  static std::string string(const llvm::json::Object &value, llvm::StringRef key) {
    auto result = value.getString(key);
    if (!result) reject("expected string: " + key.str());
    return result->str();
  }
  static int64_t integer(const llvm::json::Object &value, llvm::StringRef key, int64_t minimum) {
    auto *v = value.get(key);
    auto n = v ? v->getAsInteger() : llvm::Optional<int64_t>();
    // JSON booleans must not be interpreted as indices.
    if (!n || *n < minimum || *n > 1000000) reject("invalid integer: " + key.str());
    return *n;
  }
  static const llvm::json::Array &array(const llvm::json::Object &value, llvm::StringRef key) {
    auto *result = value.getArray(key);
    if (!result) reject("expected array: " + key.str());
    return *result;
  }
  static double number(const llvm::json::Value &value) {
    const auto &expr = object(value);
    fields(expr, {"op", "value"});
    if (string(expr, "op") != "const") reject("bind all expressions before native compilation");
    auto n = expr.getNumber("value");
    if (!n || !std::isfinite(*n)) reject("expected finite real constant");
    return *n;
  }
  static std::vector<int64_t> indices(const llvm::json::Object &value,
                                     llvm::StringRef key, int64_t limit) {
    std::vector<int64_t> result;
    std::set<int64_t> unique;
    for (const auto &index : array(value, key)) {
      auto n = index.getAsInteger();
      if (!n || *n < 0 || *n >= limit || !unique.insert(*n).second)
        reject("invalid or aliased resource: " + key.str());
      result.push_back(*n);
    }
    return result;
  }
public:
  explicit ProgramJsonGenerator(mlir::MLIRContext &ctx) : QuantumMLIRGenerator(ctx) {
    m_module = mlir::ModuleOp::create(builder.getUnknownLoc());
  }
  void initialize_mlirgen(bool entry = true, const std::string name = "",
                          std::map<std::string, std::string> args = {}) override {
    if (entry) reject("v0.1 requires -no-entrypoint; execution is managed by the Runtime");
    if (args.count("qpu") && args.at("qpu") != "qasm-backend")
      reject("v0.1 native output supports qasm-backend only");
    file_name = name;
    initCommonOpaqueTypes();
    function = mlir::FuncOp::create(builder.getUnknownLoc(), "__internal_mlir_" + name,
                                  builder.getFunctionType({}, builder.getI32Type()));
    main_entry_block = function.addEntryBlock();
    builder.setInsertionPointToStart(main_entry_block);
    m_module.push_back(function);
    function_names.push_back(function.getName().str());
  }
  void mlirgen(const std::string &source) override {
    auto parsed = llvm::json::parse(source);
    if (!parsed) reject(llvm::toString(parsed.takeError()));
    const auto &root = object(*parsed);
    fields(root, {"schema_version", "kind", "name", "num_qubits", "num_clbits", "parameters",
                  "operations", "global_phase", "registers", "source"});
    if (string(root, "schema_version") != "0.1" || string(root, "kind") != "qllvm.program")
      reject("unsupported Program contract version");
    if (string(root, "name").empty()) reject("empty program name");
    if (!array(root, "parameters").empty()) reject("unbound parameters require specialization");
    int64_t nq = integer(root, "num_qubits", 1), nc = integer(root, "num_clbits", 0);
    const auto &origin = object(*root.get("source"));
    fields(origin, {"frontend", "version", "capture_scope"});
    string(origin, "frontend"); string(origin, "version");
    if (string(origin, "capture_scope") != "circuit_only") reject("unsupported capture scope");
    std::set<std::string> registers;
    for (const auto &r : array(root, "registers")) {
      const auto &reg = object(r);
      fields(reg, {"name", "bits"});
      auto name = string(reg, "name");
      if (name.empty() || !registers.insert(name).second) reject("duplicate or empty register name");
      indices(reg, "bits", nc);
    }
    m_module->setAttr("qllvm.program_version", builder.getStringAttr("0.1"));
    m_module->setAttr("qllvm.capture_scope", builder.getStringAttr("circuit_only"));
    m_module->setAttr("qllvm.global_phase_radians", builder.getF64FloatAttr(number(*root.get("global_phase"))));
    auto loc = builder.getUnknownLoc();
    auto alloc = builder.create<mlir::quantum::QallocOp>(loc, array_type,
                             builder.getI64IntegerAttr(nq), builder.getStringAttr("q"));
    std::vector<mlir::Value> qubits;
    for (int64_t i = 0; i < nq; ++i) {
      auto idx = builder.create<mlir::ConstantOp>(loc, builder.getI64IntegerAttr(i));
      qubits.push_back(builder.create<mlir::quantum::ExtractQubitOp>(loc, qubit_type, alloc.qubits(), idx));
    }
    mlir::Value classical;
    if (nc) {
      auto type = mlir::MemRefType::get({nc}, builder.getI1Type());
      classical = builder.create<mlir::AllocaOp>(loc, type);
      auto zero = builder.create<mlir::ConstantOp>(loc, builder.getBoolAttr(false));
      for (int64_t i = 0; i < nc; ++i) {
        auto idx = builder.create<mlir::ConstantIndexOp>(loc, i);
        builder.create<mlir::StoreOp>(loc, zero, classical, mlir::ValueRange{idx});
      }
    }
    const std::map<std::string, std::pair<unsigned, unsigned>> gates = {
      {"id",{1,0}}, {"h",{1,0}}, {"x",{1,0}}, {"y",{1,0}}, {"z",{1,0}},
      {"s",{1,0}}, {"sdg",{1,0}}, {"t",{1,0}}, {"tdg",{1,0}}, {"sx",{1,0}},
      {"rx",{1,1}}, {"ry",{1,1}}, {"rz",{1,1}}, {"p",{1,1}}, {"u3",{1,3}},
      {"cx",{2,0}}, {"cz",{2,0}}, {"swap",{2,0}}, {"cp",{2,1}},
      {"measure",{1,0}}, {"reset",{1,0}}};
    const auto &operations = array(root, "operations");
    if (operations.size() > 1000000) reject("operation limit exceeded");
    for (const auto &value : operations) {
      const auto &op = object(value);
      fields(op, {"name", "qubits", "clbits", "params", "source"});
      auto name = string(op, "name");
      auto where = string(op, "source");
      auto location = mlir::FileLineColLoc::get(where, 0, 0, &context);
      auto qs = indices(op, "qubits", nq), cs = indices(op, "clbits", nc);
      const auto &ps = array(op, "params");
      auto gate = gates.find(name);
      if (gate == gates.end() && name != "barrier") reject("unsupported operation " + name + " at " + where);
      auto arity = name == "barrier" ? std::make_pair(unsigned(qs.size()), 0u) : gate->second;
      if (qs.empty() || qs.size() != arity.first || ps.size() != arity.second ||
          cs.size() != (name == "measure" ? 1u : 0u)) reject("invalid arity at " + where);
      if (name == "id") continue; // Explicitly supported identity, no QIS call required.
      std::vector<mlir::Value> operands, parameters;
      for (auto q : qs) operands.push_back(qubits[q]);
      for (const auto &p : ps)
        parameters.push_back(builder.create<mlir::ConstantOp>(location, builder.getF64FloatAttr(number(p))));
      if (name == "measure") {
        auto measure = builder.create<mlir::quantum::InstOp>(location, result_type,
                         builder.getStringAttr("measure"), operands, parameters);
        auto bit = builder.create<mlir::quantum::ResultCastOp>(location, builder.getI1Type(), measure.bit());
        auto index = builder.create<mlir::ConstantIndexOp>(location, cs[0]);
        builder.create<mlir::StoreOp>(location, bit.bit_result(), classical, mlir::ValueRange{index});
      } else if (name == "barrier") {
        builder.create<mlir::quantum::InstOp>(location, mlir::TypeRange{}, builder.getStringAttr(name),
                                             operands, parameters);
      } else {
        std::vector<mlir::Type> types(qs.size(), qubit_type);
        auto inst = builder.create<mlir::quantum::ValueSemanticsInstOp>(location, types,
                                       builder.getStringAttr(name), operands, parameters);
        for (unsigned i = 0; i < qs.size(); ++i) qubits[qs[i]] = inst.getResult(i);
      }
    }
    builder.create<mlir::quantum::DeallocOp>(loc, alloc.qubits());
  }
  void finalize_mlirgen() override {
    auto zero = builder.create<mlir::ConstantOp>(builder.getUnknownLoc(), builder.getI32IntegerAttr(0));
    builder.create<mlir::ReturnOp>(builder.getUnknownLoc(), mlir::ValueRange{zero});
    if (mlir::failed(mlir::verify(m_module))) reject("generated MLIR failed verification");
  }
};
} // namespace qllvm
