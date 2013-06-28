set (vtkUtilitiesPythonInitializer_SOURCE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR})
set (vtkUtilitiesPythonInitializer_BINARY_DIR
  ${CMAKE_CURRENT_BINARY_DIR})
vtk_module(vtkUtilitiesPythonInitializer
  DEPENDS
    vtkWrappingPythonCore
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

set (Module_SRCS vtkPVPythonInitializer.cxx)

if (PARAVIEW_FREEZE_PYTHON)
  set (FreezePython_SRC_DIR "${ParaView_SOURCE_DIR}/ThirdParty/FreezePython")
  set (output_dir "${CMAKE_CURRENT_BINARY_DIR}/FrozenPython")

  add_custom_command(
    OUTPUT "${output_dir}/vtkFrozenParaViewPython.h"
    COMMAND ${CMAKE_COMMAND}
            -DOUTPUT_DIRECTORY:PATH="${output_dir}"
            -DPACKAGE_ROOT:PATH="${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/site-packages"
            -DPYTHON_EXECUTABLE:FILEPATH="${PYTHON_EXECUTABLE}"
            -DOUTPUT_HEADER_PREFIX:STRING="vtkFrozenParaViewPython"
            -P "${FreezePython_SRC_DIR}/vtkFreezePython.cmake"
    DEPENDS "${FreezePython_SRC_DIR}/vtkFreezePython.cmake"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Freezing Python and ParaView Python packages and modules")

    include_directories("${output_dir}")
    list(APPEND Module_SRCS "${output_dir}/vtkFrozenParaViewPython.h")
endif()

vtk_module_library(${vtk-module} ${Module_SRCS})
target_link_libraries(${vtk-module}
  LINK_PUBLIC vtkWrappingPythonCore vtkCommonCore
  LINK_PRIVATE ${dependencies})
if (PARAVIEW_FREEZE_PYTHON)
  set_property(TARGET ${vtk-module} APPEND PROPERTY COMPILE_DEFINITIONS "PARAVIEW_FREEZE_PYTHON")
  add_dependencies(${vtk-module} paraview_pyc)
endif()
unset(vtk-module)
