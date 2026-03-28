# Backend module (QIR->QASM built-in)
add_subdirectory(${CMAKE_SOURCE_DIR}/backend ${CMAKE_BINARY_DIR}/backend)
# SABRE qubit mapping pass
add_subdirectory(${CMAKE_SOURCE_DIR}/passes ${CMAKE_BINARY_DIR}/passes)

add_llvm_executable(qllvm-compile qllvm-compile.cpp CompilationPipeline.cpp)

find_package(CURL REQUIRED)

target_compile_options(qllvm-compile PUBLIC "-fexceptions")

target_compile_features(qllvm-compile
                        PUBLIC
                        cxx_std_17)
llvm_update_compile_flags(qllvm-compile)

target_include_directories(qllvm-compile PUBLIC . ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS}
${CMAKE_CURRENT_SOURCE_DIR}/../parsers/qiskit
${CMAKE_CURRENT_SOURCE_DIR}/../parsers/qcis
)

link_directories(${CMAKE_BINARY_DIR}/lib)

target_link_libraries(qllvm-compile PUBLIC MLIRQuantum quantum-to-llvm-lowering qiskit-mlir-generator qcis-mlir-generator openqasmv3-mlir-generator qllvm-backend qllvm-passes ${CURL_LIBRARIES})

set_target_properties(qllvm-compile
                        PROPERTIES INSTALL_RPATH "${MLIR_INSTALL_DIR}/lib:${CMAKE_INSTALL_PREFIX}/lib;${CMAKE_BINARY_DIR}/lib")

install(TARGETS qllvm-compile RUNTIME DESTINATION bin)
