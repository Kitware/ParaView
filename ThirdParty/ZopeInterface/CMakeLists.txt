vtk_module_impl()
vtk_module_export("")

option(PARAVIEW_USE_SYSTEM_ZOPE "Use system Zope Interface Python package" OFF)
mark_as_advanced(PARAVIEW_USE_SYSTEM_ZOPE)

if(NOT PARAVIEW_USE_SYSTEM_ZOPE)
  find_package(PythonInterp)

  include(ParaViewMacros)

  set(zope_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zope")
  set(zope_BINARY_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/site-packages/zope")

  build_python_package("zope" ${zope_SOURCE_DIR} ${zope_BINARY_DIR})

  install(DIRECTORY ${zope_BINARY_DIR}
    DESTINATION ${VTK_INSTALL_LIBRARY_DIR}/site-packages
    COMPONENT Runtime)
endif()