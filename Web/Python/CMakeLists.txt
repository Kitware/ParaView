include(vtkModuleMacrosPython)

# The RELATIVE make vtk_module_python_package() preserve the vtk/module package
# structure when building/installing. Otherwise, we'd just get the "web"
# package.
vtk_module_python_package(${vtk-module} "paraview/web"
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}")

# copy some default sources and proxies for ParaViewWeb
set(ParaViewPython_BINARY_DIR "${VTK_BUILD_PYTHON_MODULE_DIR}/paraview/web")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/defaultProxies.json"
  DESTINATION "${ParaViewPython_BINARY_DIR}"
  USE_SOURCE_PERMISSIONS)
