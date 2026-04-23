set(LIBRARY_NAME MLIRQuantumUtils)

file(GLOB SRC *.cpp)

add_library(${LIBRARY_NAME} STATIC ${SRC})

set(EIGEN_INCLUDE_PATH ${EIGEN3_INCLUDE_DIR})
target_include_directories(${LIBRARY_NAME} PUBLIC . ../../../dialect/include ${CMAKE_BINARY_DIR}/mlir/dialect/include ${EIGEN_INCLUDE_PATH} /usr/local/include/eigen)