/*******************************************************************************
 * QASM backend implementation
 *******************************************************************************/

#include "qllvm/backends/QasmBackend.hpp"
#include "backends/qasm/QirToQasm.hpp"

namespace qllvm {

bool QasmBackend::emit(llvm::Module* module, const BackendOptions& opts) {
  QirToQasmTranslator translator;
  return translator.translateToFile(module, opts.outputPath, opts.kernelName);
}

}  // namespace qllvm
