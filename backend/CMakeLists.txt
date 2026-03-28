# Backend module: QIR code generation backends
# QIR->QASM conversion is built-in (backends/qasm/QirToQasm.cpp)

llvm_map_components_to_libnames(_backend_llvm_libs support core irreader)

add_library(qllvm-backend STATIC
  BackendRegistry.cpp
  backends/QasmBackend.cpp
  backends/QirRunnerBackend.cpp
  backends/QirRunnerCompat.cpp
  backends/qasm/QirToQasm.cpp
  backends/TianyanBackend.cpp
  backends/tianyan/QirToQcis.cpp
  backends/OriginQuantumBackend.cpp
  backends/origin_quantum/QirToOriginir.cpp

)

target_include_directories(qllvm-backend PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${LLVM_INCLUDE_DIRS}
)

target_link_libraries(qllvm-backend PUBLIC
  ${_backend_llvm_libs}
)

target_compile_features(qllvm-backend PUBLIC cxx_std_17)

install(TARGETS qllvm-backend ARCHIVE DESTINATION lib)
