// (C) Copyright QCFlow 2026. Licensed under Apache-2.0.
#include "mlir/transforms/optimizations/utils/linear_reversible.hpp"
#include <iostream>

int main() {
  unsigned tested=0;
  for(unsigned width:{2u,3u}) {
    unsigned accepted=0;
    for(unsigned matrix=0;matrix<(1u<<(width*width));++matrix) {
      auto sequence=qllvm::linear::synthesize(width,matrix);
      if(!sequence)continue;
      ++accepted;
      for(unsigned input=0;input<(1u<<width);++input) {
        unsigned expected=0;
        for(unsigned row=0;row<width;++row) {
          unsigned mask=(matrix>>(row*width))&((1u<<width)-1);
          unsigned parity=0;
          for(unsigned i=0;i<width;++i)parity^=((mask>>i)&1)&((input>>i)&1);
          expected|=parity<<row;
        }
        unsigned actual=input;
        for(auto edge:*sequence)actual^=((actual>>edge.first)&1)<<edge.second;
        if(expected!=actual)return 1;
        ++tested;
      }
    }
    if(accepted!=(width==2?6u:168u))return 2;
  }
  if(qllvm::linear::synthesize(4,0)||qllvm::linear::synthesize(3,0))return 3;
  std::cout << tested << " exact basis-state checks passed for all 174 linear permutations\n";
}
