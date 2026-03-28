/*******************************************************************************
 * QIR Runner backend: emit LLVM bitcode (.bc) for runner loading
 *******************************************************************************/

#include "qllvm/backends/QirRunnerBackend.hpp"
#include "qllvm/QirRunnerCompat.hpp"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include <fstream>
#include <string>

namespace qllvm {

bool QirRunnerBackend::emit(llvm::Module* module, const BackendOptions& opts) {
  if (!module) return false;

  if (!adaptModuleForQirRunner(module, opts.kernelName))
    return false;

  std::string outPath = opts.outputPath;
  if (outPath.empty()) outPath = opts.kernelName.empty() ? "output.bc" : opts.kernelName + ".bc";
  if (outPath.find(".bc") == std::string::npos) outPath += ".bc";

  std::error_code EC;
  llvm::raw_fd_ostream OS(outPath, EC, llvm::sys::fs::OF_None);
  if (EC) {
    return false;
  }

  llvm::WriteBitcodeToFile(*module, OS);
  OS.flush();
  return true;
}

}  // namespace qllvm
