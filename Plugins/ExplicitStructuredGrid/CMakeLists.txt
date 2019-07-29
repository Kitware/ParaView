paraview_add_plugin(ExplicitStructuredGrid
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES ExplicitStructuredGrid::vtkExplicitStructuredGrid
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Filter/vtk.module"
)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
