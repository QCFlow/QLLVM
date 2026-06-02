find_package(MPI REQUIRED COMPONENTS CXX)

add_library(qllvm-mpi-runtime STATIC qllvm-mpi-runtime.cpp)
target_compile_features(qllvm-mpi-runtime PRIVATE cxx_std_17)
target_include_directories(qllvm-mpi-runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qllvm-mpi-runtime PUBLIC MPI::MPI_CXX qir-runner-runtime)

add_library(qllvm-mpiq-runtime STATIC qllvm-mpiq-runtime.cpp)
target_compile_features(qllvm-mpiq-runtime PRIVATE cxx_std_17)
target_include_directories(qllvm-mpiq-runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qllvm-mpiq-runtime PUBLIC MPI::MPI_CXX)

install(TARGETS qllvm-mpi-runtime qllvm-mpiq-runtime ARCHIVE DESTINATION lib)
install(FILES qllvm-mpi-runtime.h qllvm-mpiq-runtime.h DESTINATION include/qllvm)
