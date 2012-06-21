#    ____    _ __           ____               __    ____
#   / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
#  _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
# /___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
#
# Copyright 2012 SciberQuest Inc.
#
macro(
  generate_python_bindings
  CXX_LIB_NAME
  CXX_LIB_SOURCES)

  if(PARAVIEW_ENABLE_PYTHON)

    set(BUILD_SHARED_LIBS TRUE)
    set(VTK_WRAP_PYTHON_FIND_LIBS 1)
    set(VTK_WRAP_INCLUDE_DIRS ${VTK_INCLUDE_DIR})
    include("${VTK_CMAKE_DIR}/vtkWrapPython.cmake")
    include_directories(${PYTHON_INCLUDE_PATH})

    VTK_WRAP_PYTHON3(
      ${CXX_LIB_NAME}Python
      Python_SRCS
      "${CXX_LIB_SOURCES}")

    add_library(
      ${CXX_LIB_NAME}PythonD
      ${Python_SRCS}
      ${Kit_PYTHON_EXTRA_SRCS})

    PYTHON_ADD_MODULE(
      ${CXX_LIB_NAME}Python
      ${CXX_LIB_NAME}PythonInit.cxx)

    target_link_libraries(
      ${CXX_LIB_NAME}PythonD
      ${CXX_LIB_NAME})

    if(WIN32 AND NOT CYGWIN)
      set_target_properties(
        ${CXX_LIB_NAME}Python
        PROPERTIES
        SUFFIX ".pyd")
    endif()

    install(TARGETS
      ${CXX_LIB_NAME}Python
      ${CXX_LIB_NAME}PythonD
      DESTINATION ${CMAKE_INSTALL_PREFIX})

  endif()

endmacro()
