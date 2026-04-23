# QIR runtime stub for hybrid LLVM linking (no full QIR runtime library)
add_library(qir-qrt-stub SHARED qir-qrt-stub.cpp)
target_compile_features(qir-qrt-stub PRIVATE cxx_std_17)
if(APPLE)
  set_target_properties(qir-qrt-stub PROPERTIES
    INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"
    LINK_FLAGS "-undefined dynamic_lookup")
else()
  set_target_properties(qir-qrt-stub PROPERTIES
    INSTALL_RPATH "$ORIGIN:${CMAKE_INSTALL_PREFIX}/lib"
    LINK_FLAGS "-shared")
endif()
install(TARGETS qir-qrt-stub LIBRARY DESTINATION lib)
