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
#include "gen_qasm.hpp"
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
#include <iomanip>
#include <fstream>
#include "utils/op.hpp"

using namespace std::tr1;
using namespace mlir;
namespace qllvm {

std::pair<std::string, int64_t> getbit_from_ResultCastOp(mlir::quantum::ResultCastOp op){
    auto qubit = op.measure_result();
    auto defop = qubit.getDefiningOp();
    if(defop){
        if (auto a = dyn_cast_or_null<mlir::quantum::InstOp>(defop)) {
            for( mlir::Value bit : a.qubits()){
                auto def_op = bit.dyn_cast_or_null<mlir::OpResult>();
                auto defop1 = def_op.getOwner();
                if(defop1){
                    if (auto a1 = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(defop1)) {
                        if(a1.qubits().size()==1){
                            return OP::getbit_from_valueSemanticsInstOp(a1);
                        }else{
                            assert(a1.qubits().size()!=1);
                            int index = -1;
                            for(int i = 0;i<a1.result().size();i++){
                                if(a1.result()[i] == bit){
                                    index = i;
                                    break;
                                }
                            }
                            return OP::getbit_from_muti_valueSemanticsInstOp(a1,index);
                        }
                    }else if (auto a = dyn_cast_or_null<mlir::quantum::ExtractQubitOp>(defop1)){
                        return OP::getbit_from_extractQubitOp(a);
                    }
                }
            }
        }
    }
}

// generate QASM file according to the MLIR program
void gen_qasm(mlir::ModuleOp module_ops,std::string qasm_name) {
    std::vector<std::string> qasm;
    qasm.emplace_back("OPENQASM 2.0;\n");
    qasm.emplace_back(std::string("include \"qelib1.inc\";\n"));
    int64_t current_bit = -1;
    
    std::vector<std::pair<std::string, int64_t>> measure_bits;
    std::vector<int64_t> store_cbit;
    module_ops.walk([&](mlir::StoreOp op) {
        auto bit = op.getValueToStore();
        auto def_op = bit.dyn_cast_or_null<mlir::OpResult>();
        mlir::Operation *owner = def_op.getOwner();
        if(owner){
            if (auto a = dyn_cast_or_null<mlir::quantum::ResultCastOp>(owner)){
                auto bit_tmp =getbit_from_ResultCastOp(a);
                measure_bits.emplace_back(bit_tmp);
                auto operands = op.getODSOperands(/*index=*/2);
                // ensure that the operands are not empty and only have one
                if (!operands.empty() && operands.size() == 1) {
                    ::mlir::Value c = operands.front();
                    auto def_op = c.getDefiningOp();
                    if (auto const_def_op = dyn_cast_or_null<mlir::ConstantIndexOp >(def_op)) {
                        int64_t bit_val = const_def_op.getValue();
                        store_cbit.emplace_back(bit_val);
                    }
                }
            }
        }
    });

    int num_qbit = -1;
    module_ops.walk([&](mlir::quantum::QallocOp op) {
        num_qbit = op.size().getLimitedValue();
        qasm.emplace_back(std::string( "qreg "+op.name().str()+"["+ std::to_string(num_qbit) + "];\n"));
    });
    
    if (measure_bits.size() != 0){
        qasm.emplace_back(std::string("creg c[")+ std::to_string(measure_bits.size()) + "];\n");
    }
    module_ops.walk([&](mlir::quantum::ValueSemanticsInstOp op) {
        std::string inst_name = op.name().str();
        if (op.qubits().size() == 1) {
            auto bit_map = OP::getbit_from_valueSemanticsInstOp(op);
            std::string q_name = bit_map.first;
            int64_t current_bit = bit_map.second;
            //if it is a single-qubit rotation gate
            if(op.getOperands().size() == 1){
                qasm.emplace_back(std::string(inst_name + " "+q_name+"[" + std::to_string(current_bit) + "];\n"));
                return;
            }
            if(op.getOperands().size() == 2){
                const auto angle = OP::tryGetConstAngle(op.getOperand(1));
                qasm.emplace_back(std::string(inst_name + "(" + std::to_string(angle) +") "+q_name+"["+std::to_string(current_bit)+"];\n"));
                return;
            }
            if(op.getOperands().size() == 3){
                const auto angle1= OP::tryGetConstAngle(op.getOperand(1));
                const auto angle2= OP::tryGetConstAngle(op.getOperand(2));
                qasm.emplace_back(std::string(inst_name + "(" + std::to_string(angle1) +", "+ std::to_string(angle2) + ") "+q_name+"[" + std::to_string(current_bit) + "];\n"));
                return;
            }
            if(op.getOperands().size() == 4){
                const auto angle1= OP::tryGetConstAngle(op.getOperand(1));
                const auto angle2= OP::tryGetConstAngle(op.getOperand(2));
                const auto angle3 = OP::tryGetConstAngle(op.getOperand(3));
                qasm.emplace_back(std::string(inst_name + "(" + std::to_string(angle1) +", "+ std::to_string(angle2) + ", " + std::to_string(angle3) +") "+q_name+"[" + std::to_string(current_bit) + "];\n"));
                return;
            }
        }else if(op.qubits().size() == 2){
            if(op.getOperands().size() == 2){
                auto bits_map = OP::getbit_from_muti_valueSemanticsInstOp(op);
                assert(bits_map.size() == 2);
                qasm.emplace_back(std::string(inst_name + " "+ bits_map[0].first +"["+std::to_string(bits_map[0].second)+"], "+bits_map[1].first+"["+ std::to_string(bits_map[1].second)+"];\n"));
                return;
            }
            if(op.getOperands().size() == 3){//one parameter
                auto inst_name_ref = op.name();
                if (inst_name_ref != "cphase" && inst_name_ref != "cp" && inst_name_ref != "crz"&& inst_name_ref != "cry"&& inst_name_ref != "crx" && inst_name_ref != "cu1" && inst_name_ref != "rzz" && inst_name_ref != "rxx") {
                    return;
                }
                auto bits_map = OP::getbit_from_muti_valueSemanticsInstOp(op);
                assert(bits_map.size() == 2);
                const auto angle = OP::tryGetConstAngle(op.getOperand(2));
                qasm.emplace_back(std::string(inst_name + "(" + std::to_string(angle) + ") "+ bits_map[0].first +"[" + std::to_string(bits_map[0].second) + "], "+ bits_map[1].first +"[" + std::to_string(bits_map[1].second) + "];\n"));

            }if(op.getOperands().size() == 5){//three parameters, cu3
                auto inst_name_ref = op.name();
                if (inst_name_ref != "cu3" && inst_name_ref != "cu") {
                    return;
                }
                auto bits_map = OP::getbit_from_muti_valueSemanticsInstOp(op);
                assert(bits_map.size() == 2);
                const auto angle1 = OP::tryGetConstAngle(op.getOperand(2));
                const auto angle2 = OP::tryGetConstAngle(op.getOperand(3));
                const auto angle3 = OP::tryGetConstAngle(op.getOperand(4));
                qasm.emplace_back(std::string(inst_name + "(" + std::to_string(angle1) +", "+ std::to_string(angle2) + ", " + std::to_string(angle3) +") "+ bits_map[0].first +"[" + std::to_string(bits_map[0].second) + "], "+ bits_map[1].first +"[" + std::to_string(bits_map[1].second) + "];\n"));
            }
        }else if(op.qubits().size() == 3){
            //ccx, cswap, 
            auto inst_name_ref = op.name();
            if (inst_name_ref != "cswap" && inst_name_ref != "ccx") {
                return;
            }
            auto bits_map = OP::getbit_from_muti_valueSemanticsInstOp(op);
            assert(bits_map.size() == 3);
            qasm.emplace_back(std::string(inst_name + " "+ bits_map[0].first +"["+std::to_string(bits_map[0].second)+"], "+ bits_map[1].first +"["+std::to_string(bits_map[1].second)+"], "+bits_map[2].first+"["+ std::to_string(bits_map[2].second)+"];\n"));
        }else{
            auto inst_name = op.name().str();
            std::cout<<"Not support: "<<inst_name<<" gate\n";
            exit(0);
        }
    });
    // module_ops.dump();
    if (measure_bits.size() != 0)
        for(int i = 0;i < measure_bits.size(); i++){
            qasm.emplace_back(std::string("measure " + measure_bits[i].first + "["+ std::to_string(measure_bits[i].second)+"] -> c["+std::to_string(store_cbit[i])+"];\n"));
        }

    assert(qasm_name != "");
    std::ofstream outputFile(qasm_name, std::ios::trunc);
    if (outputFile.is_open()) {
        for (auto a : qasm)
            outputFile << std::fixed << std::setprecision(15) << a;
    }
    outputFile.close();
}
} // namespace qllvm
