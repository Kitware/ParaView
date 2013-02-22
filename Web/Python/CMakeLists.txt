vtk_module_impl()
vtk_module_export("")


find_package(PythonInterp)

include(ParaViewMacros)

set(ParaViewPython_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(ParaViewPython_BINARY_DIR
  "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/site-packages/paraview")

build_python_package("ParaViewWebPython" ${ParaViewPython_SOURCE_DIR}
  ${ParaViewPython_BINARY_DIR})

install(DIRECTORY ${ParaViewPython_BINARY_DIR}
  DESTINATION ${VTK_INSTALL_LIBRARY_DIR}/site-packages
  COMPONENT Runtime)
