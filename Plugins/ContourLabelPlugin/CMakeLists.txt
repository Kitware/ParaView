paraview_add_plugin(ContourLabelPlugin
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION       "1.0"
  MODULES       ContourLabelRepresentation::ContourLabelRepresentation
  MODULE_FILES  "${CMAKE_CURRENT_SOURCE_DIR}/Representations/vtk.module"
  XML_DOCUMENTATION OFF)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
