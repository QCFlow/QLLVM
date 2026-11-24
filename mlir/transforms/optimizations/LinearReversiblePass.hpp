// (C) Copyright QCFlow 2026. Licensed under Apache-2.0.
// Modified 2026-11-24: bounded exact three-wire optimization before lowering.
#pragma once
#include "Quantum/QuantumOps.h"
#include "utils/linear_reversible.hpp"
#include "utils/mutation_tracking.hpp"
#include "utils/op.hpp"
#include "mlir/Dialect/StandardOps/IR/Ops.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/DenseMap.h"

namespace qllvm {
struct LinearReversiblePass
    : public mlir::PassWrapper<LinearReversiblePass,mlir::OperationPass<mlir::ModuleOp>> {
  using Gate=mlir::quantum::ValueSemanticsInstOp;
  using Wire=std::pair<std::string,int64_t>;
  using WireTable=llvm::DenseMap<mlir::Operation *,std::array<Wire,2>>;
  void optimize(std::vector<Gate> &gates,const WireTable &wireTable) {
    if(gates.size()<3)return;
    std::vector<Wire> support;
    std::vector<std::array<unsigned,2>> axes;
    llvm::DenseSet<mlir::Operation *> members;
    for(auto gate:gates)members.insert(gate);
    for(auto gate:gates){
      std::array<unsigned,2> local;
      for(unsigned i=0;i<2;++i){
        const auto &wire=wireTable.find(gate.getOperation())->second[i];
        auto found=std::find(support.begin(),support.end(),wire);
        if(found==support.end()){support.push_back(wire);found=support.end()-1;}
        local[i]=std::distance(support.begin(),found);
      }
      if(local[0]==local[1]||support.size()>3)return;
      axes.push_back(local);
    }
    const unsigned width=support.size();
    mlir::Value inputs[3],outputs[3];
    unsigned state=linear::identity(width);
    for(size_t i=0;i<gates.size();++i){
      auto gate=gates[i];
      for(unsigned j=0;j<2;++j){
        const unsigned axis=axes[i][j];
        if(!inputs[axis]){
          if(members.count(gate.getOperand(j).getDefiningOp()))return;
          inputs[axis]=gate.getOperand(j);
        }else if(outputs[axis]!=gate.getOperand(j))return;
        outputs[axis]=gate.getResult(j);
      }
      state=linear::appendCX(state,width,axes[i][0],axes[i][1]);
    }
    auto sequence=linear::synthesize(width,state);
    if(!sequence||sequence->size()>=gates.size())return;
    auto anchor=gates.back();
    for(size_t i=0;i<gates.size();++i)for(unsigned j=0;j<2;++j)
      for(auto *user:gates[i].getResult(j).getUsers())
        if(!members.count(user)&&
           (gates[i].getResult(j)!=outputs[axes[i][j]]||user->getBlock()!=anchor->getBlock()||
            !anchor->isBeforeInBlock(user)))return;
    mlir::OpBuilder builder(anchor);builder.setInsertionPointAfter(anchor);
    mlir::Value latest[3]={inputs[0],inputs[1],inputs[2]};
    for(const auto &edge:*sequence){
      std::vector<mlir::Value> operands{latest[edge.first],latest[edge.second]};
      std::vector<mlir::Type> types{operands[0].getType(),operands[1].getType()};
      auto gate=builder.create<Gate>(anchor.getLoc(),llvm::makeArrayRef(types),"cx",
                                     llvm::makeArrayRef(operands),llvm::None);
      latest[edge.first]=gate.getResult(0);latest[edge.second]=gate.getResult(1);
    }
    for(unsigned i=0;i<width;++i)outputs[i].replaceAllUsesWith(latest[i]);
    for(auto gate:gates)gate->dropAllUses();
    for(auto gate:gates)gate.erase();
    optimization::markChanged();
  }
  void runOnOperation() final {
    getOperation().walk([&](mlir::FuncOp function){
      for(auto &block:function.getBody()){
        std::vector<std::vector<Gate>> windows;
        std::vector<Gate> current;
        std::vector<Wire> support;
        WireTable wireTable;
        auto flush=[&](){if(!current.empty())windows.push_back(std::move(current));current.clear();support.clear();};
        {
        OP::QuantumWireResolver resolver;
        for(auto &operation:block){
          auto gate=mlir::dyn_cast<Gate>(&operation);
          if(!gate||gate.name()!="cx"||gate.getNumResults()!=2){
            if(!mlir::isa<mlir::ConstantOp,mlir::quantum::ExtractQubitOp>(&operation))flush();
            continue;
          }
          std::array<Wire,2> resolved{{resolver.resolve(gate.getOperand(0)),resolver.resolve(gate.getOperand(1))}};
          wireTable[gate.getOperation()]=resolved;
          std::vector<Wire> next=support;
          for(const auto &wire:resolved){
            if(std::find(next.begin(),next.end(),wire)==next.end())next.push_back(wire);
          }
          if(next.size()>3||current.size()==64){
            flush();next.clear();
            next.assign(resolved.begin(),resolved.end());
          }
          support=std::move(next);current.push_back(gate);
        }
        flush();
        } // Discard the SSA resolver before rewriting. Only wire names/indices remain.
        // Original operation descriptors stay valid for disjoint windows;
        // replacements preserve each logical wire's boundary mapping. Avoid
        // re-tracing a deep circuit prefix separately for every short window.
        for(auto &window:windows)optimize(window,wireTable);
      }
    });
  }
};
} // namespace qllvm
