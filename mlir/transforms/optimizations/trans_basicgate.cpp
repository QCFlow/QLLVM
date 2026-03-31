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
#include "trans_basicgate.hpp"
#include "Quantum/QuantumOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/StandardOps/IR/Ops.h"
#include "mlir/IR/Matchers.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Target/LLVMIR.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Transforms/Passes.h"
#include <iostream>
#include <string>
#include <utility>
#include <cassert>
#include "utils/circuit.hpp"
#include "utils/op.hpp"
#include "utils/gate_matrix.hpp"
#include "utils/get_matrix.hpp"
#include "gen_qasm.hpp"
#include <Eigen/Dense>
#include <eigen3/unsupported/Eigen/KroneckerProduct>
#include <eigen3/unsupported/Eigen/MatrixFunctions>
#include <unordered_map>
#include <tr1/unordered_map>
#include <cmath>
#include <cstdio>

namespace qllvm {
using namespace std::complex_literals;
void trans_basicgate::getDependentDialects(DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}

std::vector<mlir::quantum::ValueSemanticsInstOp> gate_to_trans;
std::unordered_set<std::string> basic_gate_set;

void h_to_ry_rx(mlir::quantum::ValueSemanticsInstOp &op){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;
    auto theta_val_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));
    auto theta_val_PI = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI));

    param_value.clear();
    param_value.emplace_back(theta_val_PI_2);                  
    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "ry", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);

    
    if(basic_gate_set.find("x") != basic_gate_set.end()){
        auto new_gate_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "x", llvm::makeArrayRef({inputQubit_0}),
            llvm::None);
        inputQubit_0 = new_gate_2.getResult(0);
    }else if(basic_gate_set.find("rx") != basic_gate_set.end()){
        param_value.clear();
        param_value.emplace_back(theta_val_PI);
        auto new_gate_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rx", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
        inputQubit_0 = new_gate_2.getResult(0);
    }        
    op.getResult(0).replaceAllUsesWith(inputQubit_0);
}

void ry_to_u3(mlir::quantum::ValueSemanticsInstOp &op){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value theta = op.getOperand(1);

    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;

    auto theta_val_zero = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 0.0));

    param_value.clear();
    param_value.emplace_back(theta);
    param_value.emplace_back(theta_val_zero);
    param_value.emplace_back(theta_val_zero);
                   
    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "u3", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);

    op.getResult(0).replaceAllUsesWith(inputQubit_0);     
}

void ry_to_rx_rz(mlir::quantum::ValueSemanticsInstOp &op){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value theta = op.getOperand(1);
    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;

    auto theta_val_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));
    auto theta_val_neg_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_2));

    param_value.clear();
    param_value.emplace_back(theta_val_PI_2);
                   
    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rx", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);
    auto new_gate_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rz", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef({theta}));
    inputQubit_0 = new_gate_2.getResult(0);
    
    param_value.clear();
    param_value.emplace_back(theta_val_neg_PI_2);
    auto new_gate_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rx", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_3.getResult(0);

    op.getResult(0).replaceAllUsesWith(inputQubit_0);     
}

void ry_to_sx_rz_x(mlir::quantum::ValueSemanticsInstOp &op){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value theta = op.getOperand(1);
    mlir::Type q_type = inputQubit_0.getType();
          
    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "sx", llvm::makeArrayRef({inputQubit_0}),
            llvm::None);
    inputQubit_0 = new_gate_1.getResult(0);
    auto new_gate_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rz", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef({theta}));
    inputQubit_0 = new_gate_2.getResult(0);

    auto new_gate_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "sx", llvm::makeArrayRef({inputQubit_0}),
            llvm::None);
    inputQubit_0 = new_gate_3.getResult(0);
    auto new_gate_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "x", llvm::makeArrayRef({inputQubit_0}),
            llvm::None);
    inputQubit_0 = new_gate_4.getResult(0);

    op.getResult(0).replaceAllUsesWith(inputQubit_0);     
}


void trans_ry(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    dead_ops.emplace_back(op);
    if(basic_gate_set.find("rx") != basic_gate_set.end() && basic_gate_set.find("rz") != basic_gate_set.end()){
        ry_to_rx_rz(op);
    }else if(basic_gate_set.find("rz") != basic_gate_set.end() && basic_gate_set.find("sx") != basic_gate_set.end()&& basic_gate_set.find("x") != basic_gate_set.end()){
        ry_to_sx_rz_x(op);
    }else if(basic_gate_set.find("u3") != basic_gate_set.end()){
        ry_to_u3(op);
    }else{
        assert(false && "Not support to trans Rx to the given basic gate");
    }
}

void h_to_ry_rz(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;
    auto theta_val_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));
    auto theta_val_PI = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI));
                 
    if(basic_gate_set.find("z") != basic_gate_set.end()){
        auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "z", llvm::makeArrayRef({inputQubit_0}),
            llvm::None);
        inputQubit_0 = new_gate_1.getResult(0);
    }else if(basic_gate_set.find("rz") != basic_gate_set.end()){
        param_value.clear();
        param_value.emplace_back(theta_val_PI);
        auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rz", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
        inputQubit_0 = new_gate_1.getResult(0);
    }

    param_value.clear();
    param_value.emplace_back(theta_val_PI_2); 
    auto new_gate_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type}),
        "ry", llvm::makeArrayRef({inputQubit_0}),
        llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_2.getResult(0);
    op.getResult(0).replaceAllUsesWith(inputQubit_0);
    if(basic_gate_set.find("ry") == basic_gate_set.end()){
        dead_ops.emplace_back(new_gate_2);
        ry_to_sx_rz_x(new_gate_2);
    }
}

void h_to_rx_rz(mlir::quantum::ValueSemanticsInstOp &op){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;
    auto theta_val_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));
                 
    param_value.clear();
    param_value.emplace_back(theta_val_PI_2);
    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type}),
        "rz", llvm::makeArrayRef({inputQubit_0}),
        llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);
    auto new_gate_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type}),
        "rx", llvm::makeArrayRef({inputQubit_0}),
        llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_2.getResult(0);
    auto new_gate_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type}),
        "rz", llvm::makeArrayRef({inputQubit_0}),
        llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_3.getResult(0);

    op.getResult(0).replaceAllUsesWith(inputQubit_0);
}

void h_to_u3(mlir::quantum::ValueSemanticsInstOp &op){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;
    auto theta_val_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));
    auto theta_val_PI = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI));
    auto theta_val_zero = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 0.0));

    param_value.clear();
    param_value.emplace_back(theta_val_PI_2);
    param_value.emplace_back(theta_val_zero);
    param_value.emplace_back(theta_val_PI);

    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type}),
        "u3", llvm::makeArrayRef({inputQubit_0}),
        llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);

    op.getResult(0).replaceAllUsesWith(inputQubit_0);
}

void x_to_rx(mlir::quantum::ValueSemanticsInstOp &op){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;

    auto theta_val_PI = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI));
    param_value.emplace_back(theta_val_PI);                  
    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rx", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);
    op.getResult(0).replaceAllUsesWith(inputQubit_0);
}

void x_to_u3(mlir::quantum::ValueSemanticsInstOp &op){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;

    auto theta_val_PI = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI));
    auto theta_val_zero = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 0.0));

    param_value.clear();
    param_value.emplace_back(theta_val_PI);
    param_value.emplace_back(theta_val_zero);
    param_value.emplace_back(theta_val_PI);

    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "u3", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);
    op.getResult(0).replaceAllUsesWith(inputQubit_0);
}

void x_to_ry_rz(mlir::quantum::ValueSemanticsInstOp &op){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;

    auto theta_val_PI = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI));

    param_value.clear();
    param_value.emplace_back(theta_val_PI);
                   
    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "ry", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);
    auto new_gate_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rz", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_2.getResult(0);

    op.getResult(0).replaceAllUsesWith(inputQubit_0);
}

void y_to_ry(mlir::quantum::ValueSemanticsInstOp &op){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;

    auto theta_val_PI = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI));
    param_value.emplace_back(theta_val_PI);                  
    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "ry", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));

    op.getResult(0).replaceAllUsesWith(new_gate_1.getResult(0));     
}

void y_to_u3(mlir::quantum::ValueSemanticsInstOp &op){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;

    auto theta_val_PI = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI));
    auto theta_val_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));

    param_value.clear();
    param_value.emplace_back(theta_val_PI);
    param_value.emplace_back(theta_val_PI_2);
    param_value.emplace_back(theta_val_PI_2);

    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "u3", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);
    op.getResult(0).replaceAllUsesWith(inputQubit_0);     
}

void y_to_rx_rz(mlir::quantum::ValueSemanticsInstOp &op){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;

    auto theta_val_PI = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI));

    param_value.clear();
    param_value.emplace_back(theta_val_PI);
                   
    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rz", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);
    auto new_gate_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rx", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_2.getResult(0);

    op.getResult(0).replaceAllUsesWith(inputQubit_0);     
}

void z_to_rz(mlir::quantum::ValueSemanticsInstOp &op){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;

    auto theta_val_PI = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI));
    param_value.emplace_back(theta_val_PI);                  
    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rz", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));

    op.getResult(0).replaceAllUsesWith(new_gate_1.getResult(0));
}

void z_to_u3(mlir::quantum::ValueSemanticsInstOp &op){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;

    auto theta_val_PI = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI));
    auto theta_val_zero = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 0.0));

    param_value.clear();
    param_value.emplace_back(theta_val_zero);
    param_value.emplace_back(theta_val_zero);
    param_value.emplace_back(theta_val_PI);

    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "u3", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);
    op.getResult(0).replaceAllUsesWith(inputQubit_0);     
}

void z_to_rx_ry(mlir::quantum::ValueSemanticsInstOp &op){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;

    auto theta_val_PI = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI));

    param_value.clear();
    param_value.emplace_back(theta_val_PI);
                   
    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rx", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);
    auto new_gate_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "ry", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_2.getResult(0);

    op.getResult(0).replaceAllUsesWith(inputQubit_0);     
}

void rz_to_rx_ry(mlir::quantum::ValueSemanticsInstOp &op){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value theta = op.getOperand(1);
    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;

    auto theta_val_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));
    auto theta_val_neg_PI_2 = rewriter.create<mlir::ConstantOp>(
                            op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_2));

    param_value.clear();
    param_value.emplace_back(theta_val_neg_PI_2);
                   
    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rx", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);
    auto new_gate_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "ry", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef({theta}));
    inputQubit_0 = new_gate_2.getResult(0);
    param_value.clear();
    param_value.emplace_back(theta_val_PI_2);
    auto new_gate_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rx", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_3.getResult(0);

    op.getResult(0).replaceAllUsesWith(inputQubit_0);     
}

void rz_to_u3(mlir::quantum::ValueSemanticsInstOp &op){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value theta = op.getOperand(1);

    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;

    auto theta_val_zero = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 0.0));

    param_value.clear();
    param_value.emplace_back(theta_val_zero);
    param_value.emplace_back(theta_val_zero);
    param_value.emplace_back(theta);
                   
    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "u3", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);

    op.getResult(0).replaceAllUsesWith(inputQubit_0);     
}

void rx_to_u3(mlir::quantum::ValueSemanticsInstOp &op){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value theta = op.getOperand(1);

    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;

    auto theta_val_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));
    auto theta_val_neg_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_2));

    param_value.clear();
    param_value.emplace_back(theta);
    param_value.emplace_back(theta_val_neg_PI_2);
    param_value.emplace_back(theta_val_PI_2);
    
                   
    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "u3", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);

    op.getResult(0).replaceAllUsesWith(inputQubit_0);     
}

void rx_to_ry_rz(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value theta = op.getOperand(1);
    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;

    auto theta_val_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));
    auto theta_val_neg_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_2));

    param_value.clear();
    param_value.emplace_back(theta_val_neg_PI_2);
                   
    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "ry", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);
    auto new_gate_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rz", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef({theta}));
    inputQubit_0 = new_gate_2.getResult(0);
    
    param_value.clear();
    param_value.emplace_back(theta_val_PI_2);
    auto new_gate_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "ry", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_3.getResult(0);

    op.getResult(0).replaceAllUsesWith(inputQubit_0);
    if(basic_gate_set.find("ry") == basic_gate_set.end()){
        dead_ops.emplace_back(new_gate_1);
        dead_ops.emplace_back(new_gate_3);
        ry_to_sx_rz_x(new_gate_1);
        ry_to_sx_rz_x(new_gate_3);
    }
}



void trans_h(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    dead_ops.emplace_back(op);
    if(basic_gate_set.find("ry") != basic_gate_set.end() && basic_gate_set.find("rz") != basic_gate_set.end()){
        h_to_ry_rz(op,dead_ops);
    }else if(basic_gate_set.find("rz") != basic_gate_set.end() && basic_gate_set.find("sx") != basic_gate_set.end() && basic_gate_set.find("x") != basic_gate_set.end()){
        h_to_ry_rz(op,dead_ops);
    }else if(basic_gate_set.find("rz") != basic_gate_set.end() && basic_gate_set.find("rx") != basic_gate_set.end()){
        h_to_rx_rz(op);
    }else if(basic_gate_set.find("ry") != basic_gate_set.end() && basic_gate_set.find("rx") != basic_gate_set.end()){
        h_to_ry_rx(op);
    }else if(basic_gate_set.find("u3") != basic_gate_set.end()){
        h_to_u3(op);
    }else{
        assert(false && "Not support to trans H to the given basic gate");
    }
}

void trans_rz(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    dead_ops.emplace_back(op);
    if(basic_gate_set.find("ry") != basic_gate_set.end() && basic_gate_set.find("rx") != basic_gate_set.end()){
        rz_to_rx_ry(op);
    }else if(basic_gate_set.find("u3") != basic_gate_set.end()){
        rz_to_u3(op);
    }else{
        assert(false && "Not support to trans Rz to the given basic gate");
    }
}

void trans_rx(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    dead_ops.emplace_back(op);
    if((basic_gate_set.find("ry") != basic_gate_set.end() && basic_gate_set.find("rz") != basic_gate_set.end()) || (basic_gate_set.find("rz") != basic_gate_set.end() && basic_gate_set.find("sx") != basic_gate_set.end()&& basic_gate_set.find("x") != basic_gate_set.end())){
        rx_to_ry_rz(op,dead_ops);
    }else if(basic_gate_set.find("u3") != basic_gate_set.end()){
        rx_to_u3(op);
    }else{
        assert(false && "Not support to trans Rx to the given basic gate");
    }
}



void s_sdg_t_tdg_to_rz(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    dead_ops.emplace_back(op);
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;

    auto theta_val_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));
    auto theta_val_negtive_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_2));
    auto theta_val_PI_4 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_4));
    auto theta_val_negtive_PI_4 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_4));
    
    param_value.clear();
    if(op.name() == "s"){
        param_value.emplace_back(theta_val_PI_2);
    }else if(op.name() == "sdg"){
        param_value.emplace_back(theta_val_negtive_PI_2);
    }else if(op.name() == "t"){
        param_value.emplace_back(theta_val_PI_4);
    }else if(op.name() == "tdg"){
        param_value.emplace_back(theta_val_negtive_PI_4);
    }
    auto new_gate = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rz", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    op.getResult(0).replaceAllUsesWith(new_gate.getResult(0));
    if(basic_gate_set.find("rz") == basic_gate_set.end()){
        trans_rz(new_gate,dead_ops);
    }
}

void cx_to_h_cz_h(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value inputQubit_1 = op.getOperand(1);
    std::vector<mlir::Value> param_value;
    mlir::Type q_type = op.getOperand(0).getType();

    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type}),
        "h", llvm::makeArrayRef({inputQubit_1}),
        llvm::None);
    inputQubit_1 = new_gate_1.getResult(0);
    
    auto new_gate_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type,q_type}),
        "cz", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
        llvm::None);
    inputQubit_1 = new_gate_2.getResult(1);

    auto new_gate_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type}),
        "h", llvm::makeArrayRef({inputQubit_1}),
        llvm::None);
    op.getResult(0).replaceAllUsesWith(new_gate_2.getResult(0));     
    op.getResult(1).replaceAllUsesWith(new_gate_3.getResult(0));
    if(basic_gate_set.find("h") == basic_gate_set.end()){
        trans_h(new_gate_1,dead_ops);
        trans_h(new_gate_3,dead_ops);
    }
}

void cx_to_s_cy_sdg(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value inputQubit_1 = op.getOperand(1);
    std::vector<mlir::Value> param_value;
    mlir::Type q_type = op.getOperand(0).getType();

    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type}),
        "s", llvm::makeArrayRef({inputQubit_1}),
        llvm::None);
    inputQubit_1 = new_gate_1.getResult(0);
    
    auto new_gate_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type,q_type}),
        "cy", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
        llvm::None);
    inputQubit_1 = new_gate_2.getResult(1);

    auto new_gate_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type}),
        "sdg", llvm::makeArrayRef({inputQubit_1}),
        llvm::None);
    op.getResult(0).replaceAllUsesWith(new_gate_2.getResult(0));     
    op.getResult(1).replaceAllUsesWith(new_gate_3.getResult(0));
    if(basic_gate_set.find("s") == basic_gate_set.end()){
        s_sdg_t_tdg_to_rz(new_gate_1,dead_ops);
    }
    if(basic_gate_set.find("sdg") == basic_gate_set.end()){
        s_sdg_t_tdg_to_rz(new_gate_3,dead_ops);
    }
}

void cz_to_h_cx_h(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value inputQubit_1 = op.getOperand(1);
    std::vector<mlir::Value> param_value;
    mlir::Type q_type = op.getOperand(0).getType();

    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type}),
        "h", llvm::makeArrayRef({inputQubit_1}),
        llvm::None);
    inputQubit_1 = new_gate_1.getResult(0);
    
    auto new_gate_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type,q_type}),
        "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
        llvm::None);
    inputQubit_1 = new_gate_2.getResult(1);

    auto new_gate_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type}),
        "h", llvm::makeArrayRef({inputQubit_1}),
        llvm::None);
    op.getResult(0).replaceAllUsesWith(new_gate_2.getResult(0));     
    op.getResult(1).replaceAllUsesWith(new_gate_3.getResult(0));     
}

void cz_to_rx_cy_rx(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value inputQubit_1 = op.getOperand(1);
    std::vector<mlir::Value> param_value;
    mlir::Type q_type = op.getOperand(0).getType();
    auto theta_val_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));
    auto theta_val_neg_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_2));
    param_value.clear();
    param_value.emplace_back(theta_val_neg_PI_2);                  
    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rx", llvm::makeArrayRef({inputQubit_1}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);
    auto new_gate_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type,q_type}),
        "cy", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
        llvm::None);
    inputQubit_1 = new_gate_2.getResult(1);
    inputQubit_0 = new_gate_2.getResult(0);
    param_value.clear();
    param_value.emplace_back(theta_val_PI_2);                  
    auto new_gate_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rx", llvm::makeArrayRef({inputQubit_1}),
            llvm::makeArrayRef(param_value));
    inputQubit_1 = new_gate_3.getResult(0);
    op.getResult(0).replaceAllUsesWith(inputQubit_0);     
    op.getResult(1).replaceAllUsesWith(inputQubit_1);
    if(basic_gate_set.find("rx") == basic_gate_set.end()){
        trans_rx(new_gate_1,dead_ops);
        trans_rx(new_gate_3,dead_ops);
    }
}

void cy_to_sdg_cx_s(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value inputQubit_1 = op.getOperand(1);
    std::vector<mlir::Value> param_value;
    mlir::Type q_type = op.getOperand(0).getType();

    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type}),
        "sdg", llvm::makeArrayRef({inputQubit_1}),
        llvm::None);
    inputQubit_1 = new_gate_1.getResult(0);
    
    auto new_gate_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type,q_type}),
        "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
        llvm::None);
    inputQubit_1 = new_gate_2.getResult(1);
    inputQubit_0 = new_gate_2.getResult(0);

    auto new_gate_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type}),
        "s", llvm::makeArrayRef({inputQubit_1}),
        llvm::None);
    inputQubit_1 = new_gate_3.getResult(0);
    op.getResult(0).replaceAllUsesWith(inputQubit_0);     
    op.getResult(1).replaceAllUsesWith(inputQubit_1);

    if(basic_gate_set.find("s") == basic_gate_set.end()){
        s_sdg_t_tdg_to_rz(new_gate_3,dead_ops);
    }
    if(basic_gate_set.find("sdg") == basic_gate_set.end()){
        s_sdg_t_tdg_to_rz(new_gate_1,dead_ops);
    }
}

void cy_to_rx_cz_rx(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value inputQubit_1 = op.getOperand(1);
    mlir::Type q_type = inputQubit_0.getType();
    std::vector<mlir::Value> param_value;   

    auto theta_val_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));
    auto theta_val_neg_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_2));
    param_value.clear();
    param_value.emplace_back(theta_val_PI_2);                  
    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rx", llvm::makeArrayRef({inputQubit_1}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);
    auto new_gate_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
        op.getLoc(), llvm::makeArrayRef({q_type,q_type}),
        "cz", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
        llvm::None);
    inputQubit_1 = new_gate_2.getResult(1);
    inputQubit_0 = new_gate_2.getResult(0);

    param_value.clear();
    param_value.emplace_back(theta_val_neg_PI_2);                  
    auto new_gate_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rx", llvm::makeArrayRef({inputQubit_1}),
            llvm::makeArrayRef(param_value));
    inputQubit_1 = new_gate_3.getResult(0);
    op.getResult(0).replaceAllUsesWith(inputQubit_0);     
    op.getResult(1).replaceAllUsesWith(inputQubit_1);
    if(basic_gate_set.find("rx") == basic_gate_set.end()){
        trans_rx(new_gate_1,dead_ops);
        trans_rx(new_gate_3,dead_ops);
    }
}

void trans_x(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    dead_ops.emplace_back(op);
    if(basic_gate_set.find("rx") != basic_gate_set.end()){
        x_to_rx(op);
    }else if(basic_gate_set.find("ry") != basic_gate_set.end() && basic_gate_set.find("rz") != basic_gate_set.end()){
        x_to_ry_rz(op);
    }else if(basic_gate_set.find("u3") != basic_gate_set.end()){
        x_to_u3(op);
    }else{
        assert(false && "Not support to trans X to the given basic gate");
    }
}

void trans_y(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    dead_ops.emplace_back(op);
    if(basic_gate_set.find("ry") != basic_gate_set.end()){
        y_to_ry(op);
    }else if(basic_gate_set.find("rx") != basic_gate_set.end() && basic_gate_set.find("rz") != basic_gate_set.end()){
        y_to_rx_rz(op);
    }else if(basic_gate_set.find("u3") != basic_gate_set.end()){
        y_to_u3(op);
    }else{
        assert(false && "Not support to trans Y to the given basic gate");
    }
}

void trans_z(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    dead_ops.emplace_back(op);
    if(basic_gate_set.find("rz") != basic_gate_set.end()){
        z_to_rz(op);
    }else if(basic_gate_set.find("rx") != basic_gate_set.end() && basic_gate_set.find("ry") != basic_gate_set.end()){
        z_to_rx_ry(op);
    }else if(basic_gate_set.find("u3") != basic_gate_set.end()){
        z_to_u3(op);
    }else{
        assert(false && "Not support to trans Z to the given basic gate");
    }
}

void trans_cx(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    dead_ops.emplace_back(op);
    if(basic_gate_set.find("cz") != basic_gate_set.end()){
        cx_to_h_cz_h(op,dead_ops);
    }else if(basic_gate_set.find("cy") != basic_gate_set.end()){
        cx_to_s_cy_sdg(op,dead_ops);
    }else{
        assert(false && "Not support to trans CX(CNOT) to the given basic gate");
    }
}

void trans_cz(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    dead_ops.emplace_back(op);
    if(basic_gate_set.find("cx") != basic_gate_set.end()){
        cz_to_h_cx_h(op,dead_ops);
    }else if(basic_gate_set.find("cy") == basic_gate_set.end()){
        cz_to_rx_cy_rx(op,dead_ops);
    }else{
        assert(false && "Not support to trans CY to the given basic gate");
    }
}

void trans_cy(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    dead_ops.emplace_back(op);
    if(basic_gate_set.find("cx") != basic_gate_set.end()){
        cy_to_sdg_cx_s(op,dead_ops);
    }else if(basic_gate_set.find("cz") == basic_gate_set.end()){
        cy_to_rx_cz_rx(op,dead_ops);
    }else{
        assert(false && "Not support to trans CY to the given basic gate");
    }
}

void trans_u3(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    dead_ops.emplace_back(op);
    Eigen::Matrix2cd test_matrix;
    //  computed the matrix form of the single-qubit quantum gate block
    std::string name = op.name().str();
    std::vector<double> current_op_params;
    for(int i = 1;i < 4;i++){
        auto param = op.getOperand(i);
        current_op_params.emplace_back(qllvm::OP::tryGetConstAngle(param));
    }
    auto paras = std::make_pair(name,current_op_params);
    test_matrix = qllvm::matrix::getSingleGateMat(paras);
    std::vector<std::string> basis_names;
    if(basic_gate_set.find("rz") != basic_gate_set.end() && basic_gate_set.find("ry") != basic_gate_set.end()){
        basis_names.emplace_back("ZYZ");
    }
    if(basic_gate_set.find("rx") != basic_gate_set.end() && basic_gate_set.find("ry") != basic_gate_set.end()){
        basis_names.emplace_back("XYX");
    }
    if(basic_gate_set.find("rz") != basic_gate_set.end() && basic_gate_set.find("rx") != basic_gate_set.end()){
        basis_names.emplace_back("ZXZ");
        basis_names.emplace_back("XZX");
    }
    if(basic_gate_set.find("sx") != basic_gate_set.end() && basic_gate_set.find("x") != basic_gate_set.end()){
        basis_names.emplace_back("ZSXX");
    }
    if(basic_gate_set.find("rz") != basic_gate_set.end() && basic_gate_set.find("sx") != basic_gate_set.end()){
        basis_names.emplace_back("ZSX");
    }
    if(basis_names.size() == 0){
        assert(false && "Not support to trans U3 to the given basic gate");
    }
    // if(basic_gate_set.find("phase") != basic_gate_set.end() && basic_gate_set.find("sx") != basic_gate_set.end()){
    //     basis_names.emplace_back("PSX");
    // }
    
    std::vector<qllvm::utils::EulerBasis> target_basis_set;
    for(int i = 0;i < basis_names.size();i++){
        auto basis = qllvm::utils::euler_Basis_FromStr(basis_names[i]);
        target_basis_set.emplace_back(basis);
    }
    auto simplified_seq = qllvm::utils::leastcost_basis(test_matrix,target_basis_set);
    std::vector<mlir::quantum::ValueSemanticsInstOp> new_ops;
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);

    for(auto &[pauli_inst, thetas]: simplified_seq){
        std::vector<mlir::Value> param_val;
        for(auto theta : thetas){
            mlir::Value theta_val = rewriter.create<mlir::ConstantOp>(
            op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), theta));
            param_val.emplace_back(theta_val);
        }

        std::vector<mlir::Type> ret_types{op.getOperand(0).getType()};
        if(param_val.size()!=0){
            auto new_inst = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                op.getLoc(), llvm::makeArrayRef(ret_types), pauli_inst,
                llvm::makeArrayRef(new_ops.empty() ? op.getOperand(0) : *(new_ops.back().result_begin())),
                llvm::makeArrayRef(param_val));
            new_ops.emplace_back(new_inst);
        }else{
            auto new_inst = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                op.getLoc(), llvm::makeArrayRef(ret_types), pauli_inst,
                llvm::makeArrayRef(new_ops.empty() ? op.getOperand(0) : *(new_ops.back().result_begin())),
                llvm::None);
            new_ops.emplace_back(new_inst);
        }
    }
    if(new_ops.empty()) {
        op.getResult(0).replaceAllUsesWith(op.getOperand(0));
    }else{
        auto last_inst_new = new_ops.back();
        op.getResult(0).replaceAllUsesWith(last_inst_new.getResult(0));
    }
}

void trans_cp(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    dead_ops.emplace_back(op);

    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value inputQubit_1 = op.getOperand(1);
    mlir::Type q_type = inputQubit_0.getType();

    auto lambda_v = op.getOperand(2);
    auto lambda = qllvm::OP::tryGetConstAngle(lambda_v);
    auto lambda_h_v = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), lambda/2.0));
    auto lambda_neg_h_v = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*lambda/2.0));
    std::vector<mlir::Value> param_value;

    param_value.clear();
    param_value.emplace_back(lambda_h_v);
    auto new_gate_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rz", llvm::makeArrayRef({inputQubit_0}),
            llvm::makeArrayRef(param_value));
    inputQubit_0 = new_gate_1.getResult(0);

    auto new_gate_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type,q_type}),
            "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
            llvm::None);
    inputQubit_0 = new_gate_2.getResult(0);
    inputQubit_1 = new_gate_2.getResult(1);

    param_value.clear();
    param_value.emplace_back(lambda_neg_h_v);
    auto new_gate_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rz", llvm::makeArrayRef({inputQubit_1}),
            llvm::makeArrayRef(param_value));
    inputQubit_1 = new_gate_3.getResult(0);

    auto new_gate_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type,q_type}),
            "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
            llvm::None);
    inputQubit_0 = new_gate_4.getResult(0);
    inputQubit_1 = new_gate_4.getResult(1);

    param_value.clear();
    param_value.emplace_back(lambda_h_v);
    auto new_gate_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef({q_type}),
            "rz", llvm::makeArrayRef({inputQubit_1}),
            llvm::makeArrayRef(param_value));
    inputQubit_1 = new_gate_5.getResult(0);

    op.getResult(0).replaceAllUsesWith(inputQubit_0);  
    op.getResult(1).replaceAllUsesWith(inputQubit_1);

    if(basic_gate_set.find("cx") == basic_gate_set.end()){
        trans_cx(new_gate_2,dead_ops);
        trans_cx(new_gate_4,dead_ops);
    }
    if(basic_gate_set.find("rz") == basic_gate_set.end()){
        trans_rz(new_gate_1,dead_ops);
        trans_rz(new_gate_3,dead_ops);
        trans_rz(new_gate_5,dead_ops);
    }
}

void trans_p(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit = op.getOperand(0);
  mlir::Value theta = op.getOperand(1);

  param.emplace_back(theta);
  
  mlir::Type qubit_type = inputQubit.getType();

  auto new_inst = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit}),
                      llvm::makeArrayRef(param));
  op.getResult(0).replaceAllUsesWith(*new_inst.result_begin());
  if(basic_gate_set.find("rz") == basic_gate_set.end()){
    trans_rz(new_inst,deadOps);
  }
}

void trans_sx(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit = op.getOperand(0);

  mlir::Type qubit_type = inputQubit.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "sdg", llvm::makeArrayRef({inputQubit}),
                      llvm::None);
  inputQubit =  new_inst_1.getResult(0);               
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit}),
                      llvm::None);
  inputQubit =  new_inst_2.getResult(0);
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "sdg", llvm::makeArrayRef({inputQubit}),
                      llvm::None);
  inputQubit =  new_inst_3.getResult(0);

  op.getResult(0).replaceAllUsesWith(inputQubit);

  if(basic_gate_set.find("sdg") == basic_gate_set.end() && basic_gate_set.find("rz") != basic_gate_set.end()){
    s_sdg_t_tdg_to_rz(new_inst_1,deadOps);
    s_sdg_t_tdg_to_rz(new_inst_3,deadOps);
  }else{
    assert(false && "Not support to trans SX to the given basic gate");
  }
  if(basic_gate_set.find("h") == basic_gate_set.end()){
    trans_h(new_inst_2,deadOps);
  }
}


void trans_to_basic(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
    std::string op_name = op.name().str();
    if(op_name == "x"){
        trans_x(op,dead_ops);
    }else if(op_name == "y"){
        trans_y(op,dead_ops);
    }else if(op_name == "z"){
        trans_z(op,dead_ops);
    }else if(op_name == "h"){
        trans_h(op,dead_ops);
    }else if(op_name == "cx"){
        trans_cx(op,dead_ops);
    }else if(op_name == "cz" ){
        trans_cz(op,dead_ops);
    }else if(op_name == "cy"){
        trans_cy(op,dead_ops);
    }else if(op_name == "s" || op_name == "sdg" || op_name == "t" || op_name == "tdg"){
        s_sdg_t_tdg_to_rz(op,dead_ops);
    }else if(op_name == "u3"){
        trans_u3(op,dead_ops);
    }else if(op_name == "cp"){
        trans_cp(op,dead_ops);
    }else if(op_name == "rx"){
        trans_rx(op,dead_ops);
    }else if(op_name == "ry"){
        trans_ry(op,dead_ops);
    }else if(op_name == "rz"){
        trans_rz(op,dead_ops);
    }else if(op_name == "p"){
        trans_p(op,dead_ops);
    }else if(op_name == "sx"){
        trans_sx(op,dead_ops);
    }else{
        throw std::runtime_error("Not support to trans " + op_name + " to the given basic gate");
    }
}

void trans_basicgate::runOnOperation(){
    gate_to_trans.clear();
    std::vector<mlir::quantum::ValueSemanticsInstOp> dead_ops;

    basic_gate_set = basic_gate;
    getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
        auto op_name = op.name().str();
        
        if(basic_gate_set.find(op_name) == basic_gate_set.end()){
            gate_to_trans.emplace_back(op);
        }
    });

    for(auto &elem: gate_to_trans){
        trans_to_basic(elem,dead_ops);
    }
    for(auto &elem: dead_ops){
        elem->dropAllUses();
        elem.erase();
    }

}
}