set(LIBRARY_NAME qiskit-mlir-generator)

set(ANTLR_LIB ${ANTLR4_RUNTIME})
set(ANTLR_INCLUDE ${ANTLR4_INCLUDE})
set(EXPRTK_INCLUDE_DIR ${EXPRTK_INCLUDE})

file(GLOB SRC *.cpp antlr/generated/*.cpp visitor_handlers/*.cpp utils/*.cpp)

add_library(${LIBRARY_NAME} STATIC ${SRC})

target_compile_features(${LIBRARY_NAME} 
                        PUBLIC
                        cxx_std_17)
      
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes -Wno-suggest-override -Wno-reorder -Wno-type-limits -Wno-unused-variable -Wno-unused-but-set-variable -Wno-sign-compare")
target_include_directories(
  ${LIBRARY_NAME}
  PUBLIC . .. utils visitor_handlers ../../dialect/include ${CMAKE_BINARY_DIR}/mlir/dialect/include
  antlr/generated
  ${CMAKE_CURRENT_SOURCE_DIR}/antlr/generated
  ${CMAKE_CURRENT_SOURCE_DIR}/utils
  ${CMAKE_CURRENT_SOURCE_DIR}/../qasm3  
  ${CMAKE_CURRENT_SOURCE_DIR}/../qasm3/utils  
  ${CMAKE_CURRENT_SOURCE_DIR}/../qasm3/antlr/generated 
  ${CMAKE_CURRENT_SOURCE_DIR}/../..  
  )

target_link_libraries(${LIBRARY_NAME} PUBLIC MLIRQuantum MLIRSCF MLIRIR MLIRStandard MLIRAffine openqasmv3-mlir-generator ${ANTLR_LIB})


if(APPLE)
  set_target_properties(${LIBRARY_NAME}
                        PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${MLIR_INSTALL_DIR}/lib;${LLVM_INSTALL_PREFIX}/lib")
  set_target_properties(${LIBRARY_NAME}
                        PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
else()
  set_target_properties(${LIBRARY_NAME}
                        PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib:${MLIR_INSTALL_DIR}/lib:${LLVM_INSTALL_PREFIX}/lib")
endif()
install(TARGETS ${LIBRARY_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
