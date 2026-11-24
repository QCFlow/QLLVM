// (C) Copyright QCFlow 2026. Licensed under Apache-2.0.
#include "mlir/transforms/optimizations/utils/depth_transfer.hpp"
#include <iostream>
#include <random>

int main() {
  using Block=std::vector<std::vector<size_t>>;
  std::mt19937 rng(4319);
  unsigned checked=0;
  for(unsigned sample=0;sample<500;++sample){
    Block block;
    qllvm::DepthTransfer transfer;
    for(unsigned k=0;k<12;++k){
      unsigned gate=rng()%3;
      block.push_back(gate==2?std::vector<size_t>{0,1}:std::vector<size_t>{gate});
      transfer.append(block.back());
    }
    for(int left=0;left<8;++left)for(int right=0;right<8;++right){
      std::array<int,2> actual{{left,right}};
      for(const auto &gate:block){
        int next=0;for(auto wire:gate)next=std::max(next,actual[wire]+1);
        for(auto wire:gate)actual[wire]=next;
      }
      for(unsigned out=0;out<2;++out){
        int predicted=std::max(transfer.value[out][0]+left,transfer.value[out][1]+right);
        if(predicted!=actual[out])return 1;
        ++checked;
      }
    }
  }
  qllvm::DepthTransfer a,b;
  a.append({0});b.append({0});b.append({0});
  if(!a.strictlyDominates(b)||b.strictlyDominates(a)||a.strictlyDominates(a))return 2;
  std::cout<<checked<<" arbitrary-arrival depth checks passed\n";
}
