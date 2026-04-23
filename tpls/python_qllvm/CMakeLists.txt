configure_file(qllvm.in.py
              ${CMAKE_BINARY_DIR}/qllvm.py)

# install qllvm.py to CMAKE_INSTALL_PREFIX
install(FILES ${CMAKE_BINARY_DIR}/qllvm.py DESTINATION ${CMAKE_INSTALL_PREFIX})

#Create a .pth file and install it to Python site-packages to make the qllvm module automatically importable (no need to manually set PYTHONPATH)
# Use site.USER_SITE (~/.local/lib/python3.x/site-packages) to avoid requiring root privileges
find_package(Python COMPONENTS Interpreter)
if(Python_FOUND)
  execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import site; print(site.USER_SITE)"
    OUTPUT_VARIABLE Python_USER_SITELIB
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  file(WRITE ${CMAKE_BINARY_DIR}/qllvm.pth "${CMAKE_INSTALL_PREFIX}\n")
  install(FILES ${CMAKE_BINARY_DIR}/qllvm.pth DESTINATION ${Python_USER_SITELIB})
  message(STATUS "qllvm Python module will be importable (qllvm.pth -> ${Python_USER_SITELIB})")
endif()
