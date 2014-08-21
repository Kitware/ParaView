vtk_module_impl()
vtk_module_export("")

# A little extra info on the directory structure in the source tree.
# <CURRENT_SOURCE_DIR>
#     VTK module files and CMakeLists.txt files.
# <CURRENT_SOURCE_DIR>/pygments
#     Directory corresponding to the Pygments source from pygments.org
#     stripped down to remove files we do not care about.
# <CURRENT_SOURCE_DIR>/pygments/pygments
#     The pygments Python package which gets imported in Python interpretor.
option(PARAVIEW_USE_SYSTEM_PYGMENTS "Use system Pygments Python package" OFF)
mark_as_advanced(PARAVIEW_USE_SYSTEM_PYGMENTS)

if(NOT PARAVIEW_USE_SYSTEM_PYGMENTS)
  find_package(PythonInterp)

  include(vtkPythonPackages)

  set(Pygments_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/pygments/pygments")
  set(Pygments_BINARY_DIR "${VTK_BUILD_PYTHON_MODULE_DIR}/pygments")

  build_python_package("Pygments" ${Pygments_SOURCE_DIR}
    ${Pygments_BINARY_DIR})

  install(DIRECTORY ${Pygments_BINARY_DIR}/
    DESTINATION "${VTK_INSTALL_PYTHON_MODULE_DIR}"
    COMPONENT Runtime)
endif()
