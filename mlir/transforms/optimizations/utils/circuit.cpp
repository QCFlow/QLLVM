#include "circuit.hpp"
using namespace mlir;
namespace qllvm {
namespace circuit{
int getDepth(mlir::quantum::ValueSemanticsInstOp & op){
  mlir::OpBuilder rewriter(op);
  int op_depth = op.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("depth")).getInt();
  if(op_depth != 0){
    return op_depth;
  }
  auto results = op.getResults();
  for(auto result : results){
    int childenDepth = 0;
    if (result.hasOneUse()) {
      // get that one user
      auto user = *result.user_begin();
      // cast to a inst op
      if (auto next_inst =
              dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(user)) {
                childenDepth = next_inst.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("depth")).getInt();
                if(childenDepth == 0){
                  childenDepth = getDepth(next_inst);
                }
              }else{
                if(auto OP = dyn_cast_or_null<mlir::quantum::InstOp>(user)){
                  if("mz"==OP.name()){
                    childenDepth = 1;
                  }
                }
              }
    }

    if(op_depth < childenDepth + 1 ){
      op_depth = childenDepth + 1;
    }
  }
  op.getOperation()->setAttr(llvm::StringRef("depth"),mlir::IntegerAttr::get(rewriter.getI32Type(), op_depth));
  return op_depth;
}

void getGateCountAndTopOp(int &gate_count, std::vector<mlir::quantum::ValueSemanticsInstOp> &top_op, mlir::ModuleOp ops){
    ops.walk([&](mlir::quantum::ValueSemanticsInstOp op) {
      mlir::OpBuilder rewriter(op);
      op.getOperation()->setAttr(llvm::StringRef("depth"),mlir::IntegerAttr::get(rewriter.getI32Type(), 0));
        
      auto inst_name = op.name().str();
      gate_count++;
      
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
    ops.walk([&](mlir::quantum::InstOp op) {
      if("mz"==op.name()){
        gate_count += 1;
      }
    });
}

void getGateCount(int &gate_count, mlir::ModuleOp ops){
    ops.walk([&](mlir::quantum::ValueSemanticsInstOp op) { 
      gate_count++;
    });
    ops.walk([&](mlir::quantum::InstOp op) {
      if("mz"==op.name()){
        gate_count += 1;
      }
    });
}

int getDepth(mlir::ModuleOp ops){
  std::vector<mlir::quantum::ValueSemanticsInstOp> top_op;
  int gate_count;
  int tmp_depth;
  int depth;
  getGateCountAndTopOp(gate_count, top_op, ops);
  for (auto &op : top_op) {
    tmp_depth = circuit::getDepth(op);
    depth = tmp_depth > depth ? tmp_depth : depth;
  }
  return depth;
}

int getGateNum(mlir::ModuleOp ops){
  int gate_count = 0; 
  ops.walk([&](mlir::quantum::ValueSemanticsInstOp op) { 
    gate_count++;
  });
  ops.walk([&](mlir::quantum::InstOp op) {
    if("mz"==op.name()){
      gate_count += 1;
    }
  });
  return gate_count;
}

}
}