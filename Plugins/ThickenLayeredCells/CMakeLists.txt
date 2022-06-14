paraview_add_plugin(ThickenLayeredCells
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES ThickenLayeredCells::vtkThickenLayeredCellsFilters
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Filters/vtk.module")
