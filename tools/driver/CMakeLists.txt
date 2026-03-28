# Store the location of the clang executable.
# LLVM_INSTALL_PREFIX is set by find_package(MLIR) in mlir/, which runs after tools/.
# Use LLVM_ROOT as fallback when configuring tools before mlir.
if(NOT LLVM_INSTALL_PREFIX AND LLVM_ROOT)
  set(LLVM_INSTALL_PREFIX ${LLVM_ROOT})
endif()
set(CLANG_EXECUTABLE "${LLVM_INSTALL_PREFIX}/bin/clang++")

# qir-runner-runtime.h: hybrid compile -I paths in qllvm driver
set(QLLVM_QIR_RUNTIME_INCLUDE "${CMAKE_SOURCE_DIR}/mlir/qir_runner_runtime")

configure_file(qllvm.in
               ${CMAKE_BINARY_DIR}/qllvm
               @ONLY)

install(PROGRAMS ${CMAKE_BINARY_DIR}/qllvm DESTINATION bin)
