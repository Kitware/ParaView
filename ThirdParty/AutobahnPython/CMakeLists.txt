vtk_module_impl()
vtk_module_export("")

option(PARAVIEW_USE_SYSTEM_AUTOBAHN "Use system Autobahn Python package" OFF)
mark_as_advanced(PARAVIEW_USE_SYSTEM_AUTOBAHN)

if(NOT PARAVIEW_USE_SYSTEM_AUTOBAHN)
  find_package(PythonInterp)

  include(ParaViewMacros)

  set(AutobahnPython_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/autobahn")
  set(AutobahnPython_BINARY_DIR
    "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/site-packages/autobahn")

  build_python_package("AutobahnPython" ${AutobahnPython_SOURCE_DIR}
    ${AutobahnPython_BINARY_DIR})

  install(DIRECTORY ${AutobahnPython_BINARY_DIR}
    DESTINATION ${VTK_INSTALL_LIBRARY_DIR}/site-packages
    COMPONENT Runtime)
endif()
