// Copyright QCFlow 2026. Licensed under Apache-2.0.
// Modified November 2026: exercise QIR export without upstream optimizations.
#include "backends/qasm/QirToQasm.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  llvm::LLVMContext context;
  llvm::SMDiagnostic diagnostic;
  auto module = llvm::parseIRFile(argv[1], diagnostic, context);
  if (!module) { diagnostic.print(argv[0], llvm::errs()); return 2; }
  try {
    const auto qasm = qllvm::qirToQasm(module.get());
    if (qasm.empty()) return 1;
    std::cout << qasm;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
