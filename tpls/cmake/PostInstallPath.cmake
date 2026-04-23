# After installation, add the installation directory /bin to the user's PATH
# (if it is installed in ~/.qllvm and has not been added yet)
# It is called by CMake install(SCRIPT) after ninja install.

if(NOT DEFINED ENV{HOME} OR "$ENV{HOME}" STREQUAL "")
  return()
endif()

# Only add to the PATH when installed in ~/.qllvm
get_filename_component(INSTALL_ABS "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)
set(HOME_QLLVM "$ENV{HOME}/.qllvm")
if(NOT INSTALL_ABS STREQUAL HOME_QLLVM)
  return()
endif()


foreach(RC "$ENV{HOME}/.bashrc" "$ENV{HOME}/.profile" "$ENV{HOME}/.bash_profile")
  if(EXISTS "${RC}")
    file(READ "${RC}" RC_CONTENT)
    string(FIND "${RC_CONTENT}" ".qllvm/bin" FOUND)
    if(FOUND EQUAL -1)
      file(APPEND "${RC}" "\n# qllvm (added by ninja install)\nexport PATH=\"\$PATH:$ENV{HOME}/.qllvm/bin\"\n")
      message(STATUS "Added ~/.qllvm/bin to PATH in ${RC}")
    else()
      message(STATUS "~/.qllvm/bin already in PATH (${RC})")
    endif()
    return()
  endif()
endforeach()
message(STATUS "No .bashrc/.profile found; add manually: export PATH=\"\$PATH:\$HOME/.qllvm/bin\"")
