/*******************************************************************************
 * Tianyan backend implementation
 *******************************************************************************/

#include "qllvm/backends/TianyanBackend.hpp"
#include "backends/tianyan/QirToQcis.hpp"

namespace qllvm {

bool TianyanBackend::emit(llvm::Module* module, const BackendOptions& opts) {
  QirToQcisTranslator translator;
  return translator.translateToFile(module, opts.outputPath, opts.kernelName);
}

}  // namespace qllvm
