#include "circuitState.hpp"
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
#include <unordered_map>
#include <iostream>
#include <tr1/unordered_map>
#include "utils/circuit.hpp"
#include <fstream>

using namespace std::tr1;
namespace qllvm {

void circuitState::getDependentDialects(DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}
// get the circuit state
void circuitState::runOnOperation() {  

  if (flag == true)
    *c+=1;
	
  std::unordered_map<std::string, int> gate_count;
  std::unordered_map<std::string, int>::iterator iter;
  int depth;
  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    mlir::OpBuilder rewriter(op);
    op.getOperation()->setAttr(llvm::StringRef("depth"),mlir::IntegerAttr::get(rewriter.getI32Type(), 0));
     
    auto inst_name = op.name().str();
    if(gate_count.find(inst_name) == gate_count.end())
        gate_count.emplace(inst_name,0);
    auto& counter = gate_count[inst_name];
    counter++;
    gateCount++;
    
    auto first_operand = op.getOperand(0);
    int num_operand = op.getNumResults();
    
    auto firstoperand = first_operand.dyn_cast_or_null<mlir::OpResult>();
    auto operation = firstoperand.getOwner();
    auto owner1 = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(operation);
    if(!owner1){
      top_op.emplace_back(op);
      return;
    }
    if(num_operand > 1){
      for(int i = 1; i < num_operand; i++){
        auto secondoperand = op.getOperand(i).dyn_cast_or_null<mlir::OpResult>();
        auto secondoperation = secondoperand.getOwner();
        auto owner2 = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(secondoperation);
        if(!owner2){
          top_op.emplace_back(op);
        }
      }
    }

  });
  gate_count.emplace("mz",0);
  //ValueSemantics
  getOperation().walk([&](mlir::quantum::InstOp op) {
    auto& counter = gate_count["mz"];
    if("mz"==op.name()){
      gateCount += 1;
      counter++;
    }
  });
  int cx_num = 0;
  for (auto &op : top_op) {
    depth = circuit::getDepth(op);
    ciruitDepth = depth > ciruitDepth ? depth : ciruitDepth;
  }

  std::cout <<"======================================"<<std::endl;
  std::cout << "at the beginning of the circuit, the total number of quantum gates: " << gateCount <<std::endl;
  for( iter = gate_count.begin();iter!=gate_count.end();iter++){
    std::cout << iter->first <<" number: "<< iter->second<<std::endl;
    if(iter->first == "cx"||iter->first == "cnot")
      cx_num = iter->second;
  }
  std::cout << "the depth of the circuit: " << ciruitDepth <<std::endl;
  std::cout <<"======================================"<<std::endl;

  
} 
} // namespace qllvm
