vtk_module_impl()
vtk_module_export("")

find_package(PythonInterp)

include(vtkPythonPackages)

# Set global Python module directory for build tree
set(pv_python_base_path "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/site-packages")
set_property(GLOBAL PROPERTY PV_PYMODULE_BUILD_DIR ${pv_python_base_path})

set(ParaViewPython_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(ParaViewPython_BINARY_DIR "${pv_python_base_path}/paraview/web")

build_python_package(
  "ParaViewWebPython"
  ${ParaViewPython_SOURCE_DIR}
  ${ParaViewPython_BINARY_DIR}
)

install(DIRECTORY ${ParaViewPython_BINARY_DIR}
  DESTINATION ${VTK_INSTALL_LIBRARY_DIR}/site-packages/paraview
  COMPONENT Runtime)
