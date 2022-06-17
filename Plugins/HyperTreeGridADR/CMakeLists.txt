paraview_add_plugin(HyperTreeGridADR
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES HyperTreeGridFilters
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/HyperTreeGridFilters/vtk.module"
  )
