# SABRE qubit mapping pass - LLVM IR level
llvm_map_components_to_libnames(_sabre_llvm_libs support core)

add_library(qllvm-passes STATIC
  lib/CouplingMap.cpp
  lib/CircuitExtractor.cpp
  lib/ApplyLayoutQirPass.cpp
  lib/DecomposeSwapQirPass.cpp
  lib/SabreSwap.cpp
  lib/SabreLayout.cpp
  lib/SabrePass.cpp
  lib/u3_to_rphi.cpp
)

target_include_directories(qllvm-passes PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${LLVM_INCLUDE_DIRS}
)

target_link_libraries(qllvm-passes PUBLIC ${_sabre_llvm_libs})
target_compile_features(qllvm-passes PUBLIC cxx_std_17)

install(TARGETS qllvm-passes ARCHIVE DESTINATION lib)
