include(vtkModuleMacrosPython)

# The RELATIVE make vtk_module_python_package() preserve the vtk/module package
# structure when building/installing. Otherwise, we'd just get the "web"
# package.
vtk_module_python_package(${vtk-module} "paraview/web"
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}")

# copy some default sources and proxies for ParaViewWeb
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/defaultProxies.json"
  "${VTK_BUILD_PYTHON_MODULES_DIR}/paraview/web/defaultProxies.json"
  COPYONLY)
