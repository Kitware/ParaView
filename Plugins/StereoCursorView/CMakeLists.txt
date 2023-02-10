paraview_add_plugin(StereoCursorView
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES StereoCursorView::StereoCursorViews
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/StereoCursorViews/vtk.module"
  XML_DOCUMENTATION OFF)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif()
