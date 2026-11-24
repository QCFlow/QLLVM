// (C) Copyright QCFlow 2026. Licensed under Apache-2.0.
// Max-plus dependency transfer for a closed two-wire block. Componentwise
// dominance preserves depth for arbitrary arrival times on either input wire.
#pragma once
#include <array>
#include <algorithm>
#include <vector>
namespace qllvm {
struct DepthTransfer {
  static constexpr int absent=-1000000000;
  std::array<std::array<int,2>,2> value{{{{0,absent}},{{absent,0}}}};
  void append(const std::vector<size_t> &axes) {
    std::array<int,2> next{{absent,absent}};
    for(size_t input=0;input<2;++input)
      for(size_t axis:axes)
        if(value[axis][input]!=absent)next[input]=std::max(next[input],value[axis][input]+1);
    for(size_t axis:axes)value[axis]=next;
  }
  bool strictlyDominates(const DepthTransfer &other) const {
    bool improves=false;
    for(size_t output=0;output<2;++output)for(size_t input=0;input<2;++input){
      if(value[output][input]>other.value[output][input])return false;
      improves|=value[output][input]<other.value[output][input];
    }
    return improves;
  }
};
} // namespace qllvm
