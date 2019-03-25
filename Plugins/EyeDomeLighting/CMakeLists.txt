paraview_add_plugin(EyeDomeLighting
  REQUIRED_ON_SERVER
  REQUIRED_ON_CLIENT
  VERSION "1.0"
  MODULES EyeDomeLighting::EyeDomeLightingView
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/EyeDomeLightingView/vtk.module"
  XML_DOCUMENTATION OFF)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
