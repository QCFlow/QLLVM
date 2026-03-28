/*******************************************************************************
 * QIR to OpenQASM translator - Part of qllvm backend module
 * Parses LLVM IR (QIR) and emits OpenQASM without runtime execution
 *******************************************************************************/
 #pragma once

 #include <string>
 
 namespace llvm {
 class Module;
 }
 
 namespace qllvm {
 
 /// Translator class: QIR (llvm::Module) -> OpenQASM string
 /// Encapsulates the translation logic for extensibility and testability.
 class QirToOriginirTranslator {
 public:
   QirToOriginirTranslator() = default;
   virtual ~QirToOriginirTranslator() = default;
 
   /// Translate QIR module to OpenQASM 2.0 string.
   /// @param module  LLVM module containing QIR
   /// @param kernelName  Name hint for __internal_mlir_<kernelName>, empty = auto-detect
   /// @return OpenQASM 2.0 string, or empty on failure
   virtual std::string translate(llvm::Module* module,
                                 const std::string& kernelName = "");
 
   /// Translate QIR to OpenQASM and write to file.
   /// @return true on success
   virtual bool translateToFile(llvm::Module* module,
                                const std::string& outPath,
                                const std::string& kernelName = "");
 };
 
 // --- Convenience free functions (delegate to default Translator) ---
 
 /// Convert QIR (LLVM Module) to OpenQASM string
 std::string qirToOriginir(llvm::Module* module, const std::string& kernelName = "");
 
 /// Convert QIR to OpenQASM and write to file
 bool qirToOriginirFile(llvm::Module* module, const std::string& outPath,
                    const std::string& kernelName = "");
 
 }  // namespace qllvm
 