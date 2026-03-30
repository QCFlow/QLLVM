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
using namespace mlir;
namespace qllvm {
namespace OP{

std::pair<std::string, int64_t> getbit_from_extractQubitOp(mlir::quantum::ExtractQubitOp op){

    auto b = op.idx();
    auto q = op.qreg();
    std::string q_name ="";
    auto def_op1 = q.getDefiningOp();
    if (auto const_def_op1 = dyn_cast_or_null<mlir::quantum::QallocOp>(def_op1)) {
        q_name = const_def_op1.name().str();
    }

    auto def_op = b.getDefiningOp();
    
    if (auto const_def_op = dyn_cast_or_null<mlir::ConstantIntOp>(def_op)) {
        int64_t bit_val = const_def_op.getValue();
        // std::cout << "Get constant param: " << bit_val << "\n";
        return std::make_pair(q_name, bit_val);
    }
}

std::pair<std::string, int64_t> getbit_from_valueSemanticsInstOp(mlir::quantum::ValueSemanticsInstOp op){
    auto qubit = op.qubits()[0];
    auto defop = qubit.getDefiningOp();
    if (auto a = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(defop)) {
        if(a.qubits().size()==1){
            return OP::getbit_from_valueSemanticsInstOp(a);
        }else{
            // auto def_op = qubit.dyn_cast_or_null<mlir::OpResult>();
            // auto r = def_op.getOwner();
            // if (auto const_def_op = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(r)) {
            //     assert(const_def_op.getNumResults()!=1);
                int index = -1;
                for(int i = 0;i<a.result().size();i++){
                    if(a.result()[i] == qubit){
                        index = i;
                    }
                }
                return OP::getbit_from_muti_valueSemanticsInstOp(a,index);
            // }
        }
    }else if (auto a = dyn_cast_or_null<mlir::quantum::ExtractQubitOp>(defop)){
        return OP::getbit_from_extractQubitOp(a);
    }else {
        std::cout<<"There is a problem"<<std::endl;
    }
}

std::pair<std::string, int64_t> getbit_from_muti_valueSemanticsInstOp(mlir::quantum::ValueSemanticsInstOp op,int index){
    mlir::Value qubit = op.qubits()[index];
    
    auto defop = qubit.getDefiningOp();
    if(defop)
        if (auto a = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(defop)) {
            if(a.qubits().size()==1){
                return OP::getbit_from_valueSemanticsInstOp(a);
            }else{
                // auto def_op = qubit.dyn_cast_or_null<mlir::OpResult>();
                // auto r = def_op.getOwner();
                // if (auto const_def_op = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(r)) {
                //     assert(const_def_op.getNumResults()!=1);
                    int s = -1;
                    for(int i = 0;i<a.result().size();i++){
                        if(a.result()[i] == qubit){
                            s = i;
                        }
                    }
                    return OP::getbit_from_muti_valueSemanticsInstOp(a,s);
                // }
            }
        }else if (auto a = dyn_cast_or_null<mlir::quantum::ExtractQubitOp>(defop)){
            return OP::getbit_from_extractQubitOp(a);
        }else {
            std::cout<<"There is a problem"<<std::endl;
        }
}

std::vector<std::pair<std::string, int64_t>> getbit_from_muti_valueSemanticsInstOp(mlir::quantum::ValueSemanticsInstOp op){
    std::vector<std::pair<std::string, int64_t>> bits;
    for(auto qubit : op.qubits()){
        auto defop = qubit.getDefiningOp();
        if(defop)
            if (auto a = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(defop)) {
                if(a.qubits().size()==1){
                    bits.emplace_back(OP::getbit_from_valueSemanticsInstOp(a));
                }else{
                    // auto def_op = qubit.dyn_cast_or_null<mlir::OpResult>();
                    // auto r = def_op.getOwner();
                    // if (auto const_def_op = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(r)) {
                    //     assert(const_def_op.getNumResults()!=1);
                        int index = -1;
                        for(int i = 0;i<a.result().size();i++){
                            if(a.result()[i] == qubit){
                                index = i;
                            }
                        }
                        bits.emplace_back(OP::getbit_from_muti_valueSemanticsInstOp(a,index));
                    // }
                }
            }else if (auto a = dyn_cast_or_null<mlir::quantum::ExtractQubitOp>(defop)){
                bits.emplace_back(OP::getbit_from_extractQubitOp(a));
            }else {
                std::cout<<"There is a problem"<<std::endl;
            }
    }
    return bits;
}

double tryGetConstAngle(mlir::Value theta_var){
    if (!theta_var.getType().isa<mlir::FloatType>()) {
        return 0;
    }
    // Find the defining op:
    auto def_op = theta_var.getDefiningOp();
    if (def_op) {
        // Try cast:
        if (auto const_def_op =
                dyn_cast_or_null<mlir::ConstantFloatOp>(def_op)) {
            llvm::APFloat theta_var_const_cal = const_def_op.getValue();
            double angle = theta_var_const_cal.convertToDouble();
            // std::cout << angle <<std::endl;
            return angle;
        }else if(auto const_def_op =
                dyn_cast_or_null<mlir::DivFOp>(def_op)) {
            double left_value = tryGetConstAngle(const_def_op.lhs());
            double right_value = tryGetConstAngle(const_def_op.rhs());
            assert(right_value != 0);
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
    return 0;
}

}//namespace OP
} // namespace qllvm
