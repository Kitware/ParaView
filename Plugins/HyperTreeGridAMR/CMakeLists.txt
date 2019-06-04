paraview_add_plugin(HyperTreeGridAMR
  VERSION "1.0"
  MODULES HyperTreeGridFilters
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/HyperTreeGridFilters/vtk.module"
#  PYTHON_MODULES HyperTreeGridFilters
  )
