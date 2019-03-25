paraview_add_plugin(PanoramicProjectionView
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES PanoramicProjectionView::vtkPanoramicProjectionViews
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Views/vtk.module"
  XML_DOCUMENTATION OFF)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
