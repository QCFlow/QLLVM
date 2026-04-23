# QIR Runner Runtime - spawn qir-runner subprocess for hybrid simulation
add_library(qir-runner-runtime STATIC qir-runner-runtime.cpp)
target_compile_features(qir-runner-runtime PRIVATE cxx_std_17)
target_include_directories(qir-runner-runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS qir-runner-runtime ARCHIVE DESTINATION lib)
install(FILES qir-runner-runtime.h DESTINATION include/qllvm)
