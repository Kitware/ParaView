vtk_module_impl()
vtk_module_export("")

find_package(PythonInterp)

include(vtkPythonPackages)

set(ParaViewPython_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(ParaViewPython_BINARY_DIR "${VTK_BUILD_PYTHON_MODULE_DIR}/paraview/web")

build_python_package(
  "ParaViewWebPython"
  ${ParaViewPython_SOURCE_DIR}
  ${ParaViewPython_BINARY_DIR}
)

install(DIRECTORY ${ParaViewPython_BINARY_DIR}
  DESTINATION ${VTK_INSTALL_PYTHON_MODULE_DIR}/paraview
  COMPONENT Runtime)
