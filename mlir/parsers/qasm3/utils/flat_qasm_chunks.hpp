// (C) Copyright QCFlow 2026. Licensed under Apache-2.0.
// Modified 2026-11-24: identify bounded statement chunks only for flat QASM.
// Complex declarations, control flow, modifiers and compound statements retain
// the full parser. This scanner never rewrites source or accepts invalid QASM.
#pragma once
#include <cctype>
#include <string>
#include <unordered_set>
#include <vector>

namespace qllvm {
inline std::vector<size_t> flatQasmChunks(const std::string &source) {
  if(source.size()<64*1024)return {};
  static const std::unordered_set<std::string> heads={
      "OPENQASM","include","qreg","creg","measure","reset","barrier",
      "u","u1","u2","u3","U","CX","cx","cy","cz","h","x","y","z",
      "s","sdg","t","tdg","sx","sxdg","rx","ry","rz","p","cp","cu1",
      "crx","cry","crz","cu","cu3","ch","ccx","cswap","swap","rxx","ryy","rzz"};
  enum Mode {Normal,LineComment,BlockComment,String};
  Mode mode=Normal;
  int parens=0,brackets=0;
  std::string head;
  size_t statements=0,chunkStart=0;
  size_t headerStatements=0;
  bool bodyStarted=false;
  std::vector<size_t> ends;
  for(size_t i=0;i<source.size();++i){
    const char c=source[i],next=i+1<source.size()?source[i+1]:0;
    if(mode==LineComment){if(c=='\n')mode=Normal;continue;}
    if(mode==BlockComment){if(c=='*'&&next=='/'){mode=Normal;++i;}continue;}
    if(mode==String){
      if(c=='\\'&&next){++i;continue;}
      if(c=='"')mode=Normal;
      continue;
    }
    if(c=='/'&&next=='/'){mode=LineComment;++i;continue;}
    if(c=='/'&&next=='*'){mode=BlockComment;++i;continue;}
    if(c=='"'){if(head!="include")return {};mode=String;continue;}
    if(c=='{'||c=='}'||c=='@'||c=='='||c==':')return {};
    if(head.empty()){
      if(std::isspace(static_cast<unsigned char>(c)))continue;
      if(!std::isalpha(static_cast<unsigned char>(c))&&c!='_')return {};
      const size_t first=i;
      while(i+1<source.size()&&(std::isalnum(static_cast<unsigned char>(source[i+1]))||source[i+1]=='_'))++i;
      head=source.substr(first,i-first+1);
      if(!heads.count(head))return {};
      if(head=="OPENQASM"||head=="include") {
        if(bodyStarted||++headerStatements>16)return {};
      } else bodyStarted=true;
      continue;
    }
    if(c=='(')++parens;
    if(c==')'&&--parens<0)return {};
    if(c=='[')++brackets;
    if(c==']'&&--brackets<0)return {};
    if(c==';'&&parens==0&&brackets==0){
      head.clear();
      if(++statements>=2048||(bodyStarted&&i+1-chunkStart>=512*1024)){
        ends.push_back(i+1);chunkStart=i+1;statements=0;
      }
    }
  }
  if(mode==String||mode==BlockComment||parens||brackets||!head.empty())return {};
  if(ends.empty())return {};
  if(ends.back()!=source.size())ends.push_back(source.size());
  return ends;
}
} // namespace qllvm
