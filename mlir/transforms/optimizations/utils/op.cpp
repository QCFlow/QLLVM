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
#include "op.hpp"
#include <iostream>
#include <cmath>
#include <stdexcept>
// Modified for the QLLVM correctness revision (2026): never guess unknown angles as zero.
using namespace mlir;
namespace qllvm {
namespace OP{

// Follow the actual SSA result number; malformed/unsupported handles are errors.
static std::pair<std::string, int64_t> resolveQuantumWire(
    mlir::Value value,
    llvm::DenseMap<mlir::Value, std::pair<std::string, int64_t>> *cache = nullptr) {
    std::vector<mlir::Value> path;
    auto remember = [&](const std::pair<std::string, int64_t> &wire) {
        if (cache) for (auto visited : path) (*cache)[visited] = wire;
        return wire;
    };
    for (unsigned step = 0; step < 100000; ++step) {
        if (cache) {
            auto found = cache->find(value);
            if (found != cache->end()) {
                // Copy before inserting: DenseMap growth invalidates references.
                const auto wire = found->second;
                return remember(wire);
            }
            path.push_back(value);
        }
        auto *owner = value.getDefiningOp();
        if (!owner) throw std::runtime_error("Unsupported block argument in quantum wire resolution");
        if (auto extract = dyn_cast<mlir::quantum::ExtractQubitOp>(owner)) {
            auto alloc = extract.qreg().getDefiningOp<mlir::quantum::QallocOp>();
            if (!alloc) throw std::runtime_error("Quantum register is not a fixed allocation");
            if (auto index = extract.idx().getDefiningOp<mlir::ConstantIntOp>())
                return remember({alloc.name().str(), index.getValue()});
            if (auto index = extract.idx().getDefiningOp<mlir::ConstantIndexOp>())
                return remember({alloc.name().str(), index.getValue()});
            throw std::runtime_error("Unresolved quantum register index");
        }
        if (auto gate = dyn_cast<mlir::quantum::ValueSemanticsInstOp>(owner)) {
            auto result = value.dyn_cast<mlir::OpResult>();
            if (!result || result.getResultNumber() >= gate.qubits().size())
                throw std::runtime_error("Invalid quantum SSA result index");
            value = gate.qubits()[result.getResultNumber()];
            continue;
        }
        throw std::runtime_error("Unsupported defining operation for quantum wire");
    }
    throw std::runtime_error("Quantum wire resolution exceeded its structural bound");
}

std::pair<std::string, int64_t> QuantumWireResolver::resolve(mlir::Value value) {
    return resolveQuantumWire(value, &cache);
}

std::pair<std::string, int64_t> getbit_from_extractQubitOp(mlir::quantum::ExtractQubitOp op) {
    return resolveQuantumWire(op.qbit());
}
std::pair<std::string, int64_t> getbit_from_valueSemanticsInstOp(mlir::quantum::ValueSemanticsInstOp op) {
    if (op.qubits().empty()) throw std::runtime_error("Quantum operation has no operands");
    return resolveQuantumWire(op.qubits()[0]);
}
std::pair<std::string, int64_t> getbit_from_muti_valueSemanticsInstOp(mlir::quantum::ValueSemanticsInstOp op,int index) {
    if (index < 0 || size_t(index) >= op.qubits().size())
        throw std::runtime_error("Quantum operand index is out of range");
    return resolveQuantumWire(op.qubits()[index]);
}
std::vector<std::pair<std::string, int64_t>> getbit_from_muti_valueSemanticsInstOp(mlir::quantum::ValueSemanticsInstOp op) {
    std::vector<std::pair<std::string, int64_t>> result;
    for (auto value : op.qubits()) result.push_back(resolveQuantumWire(value));
    return result;
}

double tryGetConstAngle(mlir::Value theta_var){
    if (!theta_var.getType().isa<mlir::FloatType>())
        throw std::runtime_error("Gate angle is not a supported floating-point constant");
    // Find the defining op:
    auto def_op = theta_var.getDefiningOp();
    if (def_op) {
        const auto name = def_op->getName().getStringRef();
        if (def_op->getNumOperands() == 1 &&
            (name == "std.sin" || name == "std.cos" || name == "std.sqrt" ||
             name == "std.exp" || name == "std.log" || name == "std.log2" || name == "std.atan")) {
            double x = tryGetConstAngle(def_op->getOperand(0));
            double value = name == "std.sin" ? std::sin(x) : name == "std.cos" ? std::cos(x) :
                           name == "std.sqrt" ? std::sqrt(x) : name == "std.exp" ? std::exp(x) :
                           name == "std.log" ? std::log(x) : name == "std.log2" ? std::log2(x) : std::atan(x);
            if (!std::isfinite(value)) throw std::runtime_error("Non-finite gate angle expression");
            return value;
        }
        // Try cast:
        if (auto const_def_op =
                dyn_cast_or_null<mlir::ConstantFloatOp>(def_op)) {
            llvm::APFloat theta_var_const_cal = const_def_op.getValue();
            double angle = theta_var_const_cal.convertToDouble();
            if (!std::isfinite(angle)) throw std::runtime_error("Non-finite constant gate angle");
            // std::cout << angle <<std::endl;
            return angle;
        }else if(auto const_def_op =
                dyn_cast_or_null<mlir::DivFOp>(def_op)) {
            double left_value = tryGetConstAngle(const_def_op.lhs());
            double right_value = tryGetConstAngle(const_def_op.rhs());
            if (right_value == 0) throw std::runtime_error("Division by zero in gate angle");
            return left_value / right_value;
        }else if(auto const_def_op =
                dyn_cast_or_null<mlir::MulFOp>(def_op)) {
            double left_value = tryGetConstAngle(const_def_op.lhs());
            double right_value = tryGetConstAngle(const_def_op.rhs());
            return left_value * right_value;
        }else if(auto const_def_op =
                dyn_cast_or_null<mlir::AddFOp>(def_op)) {
            double left_value = tryGetConstAngle(const_def_op.lhs());
            double right_value = tryGetConstAngle(const_def_op.rhs());
            return left_value + right_value;
        }else if(auto const_def_op =
                dyn_cast_or_null<mlir::SubFOp>(def_op)) {
            double left_value = tryGetConstAngle(const_def_op.lhs());
            double right_value = tryGetConstAngle(const_def_op.rhs());
            return left_value - right_value;
        }else if (auto const_def_op =
                dyn_cast_or_null<mlir::SIToFPOp>(def_op)){
            // DivFOp and MulFOp are the outputs of SIToFPOp.
            // SIToFPOp is used to convert an integer to a float, for example
            //  %27 = sitofp %c16_i64 : i64 to f64
            mlir::Value in = const_def_op.in();
            auto de_in_op = in.getDefiningOp();
            if (auto Int_def_op =
                dyn_cast_or_null<mlir::ConstantIntOp>(de_in_op)) {
                int64_t v = Int_def_op.getValue();
                auto pram = static_cast<double>(v);
                // std::cout << pram <<std::endl;
                return pram;
            }
        }else{
            theta_var.dump();  
        }
    }
    throw std::runtime_error("Unsupported symbolic or runtime gate angle");
}

}//namespace OP
} // namespace qllvm
