set (vtkUtilitiesPythonInitializer_SOURCE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR})
set (vtkUtilitiesWrapClientServer_BINARY_DIR
  ${CMAKE_CURRENT_BINARY_DIR})
vtk_module(vtkUtilitiesPythonInitializer
  DEPENDS
    vtkWrappingPython
    vtkCommonCore
  EXCLUDE_FROM_ALL
  EXCLUDE_FROM_WRAPPING)

set (vtk-module vtkUtilitiesPythonInitializer)
#------------------------------------------------------------------------------
# To simplify static builds with Python-enabled, we create a new library that
# manages the Python intialization necessary for static builds.
include(vtkPythonWrapping)

set (dependencies)
# when building static ParaView, generate a header file to initialize
# all the python wrapped modules statically.
vtk_write_python_modules_header_for_wrapped_modules(
  "${CMAKE_CURRENT_BINARY_DIR}/pvpythonmodules.h"
  dependencies)
include_directories(${CMAKE_CURRENT_BINARY_DIR})


vtk_module_library(${vtk-module}
  vtkPVPythonInitializer.cxx)
target_link_libraries(${vtk-module}
  LINK_PUBLIC vtkWrappingPython vtkCommonCore
  LINK_PRIVATE ${dependencies})
unset(vtk-module)
