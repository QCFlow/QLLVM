/*******************************************************************************
 * Origin quantum backend implementation
 *******************************************************************************/

#include "qllvm/backends/OriginQuantumBackend.hpp"
#include "backends/origin_quantum/QirToOriginir.hpp"
#include <iostream>

namespace qllvm {

  bool OriginQuantumBackend::emit(llvm::Module* module, const BackendOptions& opts) {
    QirToOriginirTranslator translator;
    return translator.translateToFile(module, opts.outputPath, opts.kernelName);
  }

}  // namespace qllvm
