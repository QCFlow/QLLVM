// (C) Copyright QCFlow 2026. Licensed under Apache-2.0.
// Exact all-to-all CNOT synthesis for at most three wires. The search space is
// GL(3,2), only 168 elements; no floating-point or 8x8 unitary search is used.
#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace qllvm {
namespace linear {
using CX = std::pair<unsigned,unsigned>;
inline unsigned identity(unsigned width) {
  unsigned result=0;
  for (unsigned i=0;i<width;++i) result|=(1u<<i)<<(i*width);
  return result;
}
inline unsigned appendCX(unsigned state,unsigned width,unsigned control,unsigned target) {
  const unsigned rowMask=(1u<<width)-1;
  const unsigned row=((state>>(target*width))^(state>>(control*width)))&rowMask;
  return (state&~(rowMask<<(target*width)))|(row<<(target*width));
}
struct Table {
  unsigned width;
  std::array<int,512> parent;
  std::array<CX,512> edge;
  explicit Table(unsigned width):width(width) {
    parent.fill(-1);
    const unsigned root=identity(width);parent[root]=root;
    std::vector<unsigned> queue{root};
    for (size_t head=0;head<queue.size();++head)
      for (unsigned c=0;c<width;++c) for (unsigned t=0;t<width;++t) {
        if(c==t)continue;
        const unsigned next=appendCX(queue[head],width,c,t);
        if(parent[next]>=0)continue;
        parent[next]=queue[head];edge[next]={c,t};queue.push_back(next);
      }
  }
};
inline std::optional<std::vector<CX>> synthesize(unsigned width,unsigned state) {
  if(width<2||width>3||state>=(1u<<(width*width)))return std::nullopt;
  const auto &table=[&]() -> const Table & {
    if(width==2){static const Table two(2);return two;}
    static const Table three(3);return three;
  }();
  if(table.parent[state]<0)return std::nullopt;
  std::vector<CX> result;
  const unsigned root=identity(width);
  while(state!=root){result.push_back(table.edge[state]);state=table.parent[state];}
  std::reverse(result.begin(),result.end());
  return result;
}
} // namespace linear
} // namespace qllvm
