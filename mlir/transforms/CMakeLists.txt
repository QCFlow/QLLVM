set(LIBRARY_NAME quantum-to-llvm-lowering)

file(GLOB SRC *.cpp lowering/*.cpp optimizations/*.cpp)

add_subdirectory(optimizations/utils)

get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)
get_property(conversion_libs GLOBAL PROPERTY MLIR_CONVERSION_LIBS)
set(LIBS
        ${dialect_libs}
        ${conversion_libs}
        MLIRQuantum
        MLIROptLib
        MLIRTargetLLVMIR
        MLIRExecutionEngine
        MLIRQuantumUtils
        )

# STATIC (not SHARED): avoid duplicate LLVM CommandLine globals when the same
# process loads this code via libquantum-to-llvm-lowering.so and qllvm-compile.
add_mlir_library(${LIBRARY_NAME} ${SRC} LINK_LIBS PUBLIC ${LIBS})
target_compile_features(${LIBRARY_NAME} 
                        PUBLIC
                        cxx_std_17)
target_include_directories(
  ${LIBRARY_NAME}
  PUBLIC . ../dialect/include ${CMAKE_BINARY_DIR}/mlir/dialect/include lowering/optimizations/utils)
# add_mlir_library compiles sources via target obj.${LIBRARY_NAME}; PUBLIC includes on the
# archive target are not always applied to that object target (LLVM 12).
if(TARGET obj.${LIBRARY_NAME})
  set(_qllvm_eigen_inc ${EIGEN3_INCLUDE_DIR})
  target_include_directories(obj.${LIBRARY_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../dialect/include
    ${CMAKE_BINARY_DIR}/mlir/dialect/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lowering/optimizations/utils
    ${_qllvm_eigen_inc}
    /usr/local/include/eigen)
  # Object target inherits LLVM's -fno-exceptions; optimization code uses throw/catch.
  target_compile_options(obj.${LIBRARY_NAME} PRIVATE -fexceptions)
endif()
target_compile_options(${LIBRARY_NAME} PUBLIC "-fexceptions")
