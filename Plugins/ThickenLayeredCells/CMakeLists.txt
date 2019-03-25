paraview_add_plugin(ThickenLayeredCells
  VERSION "1.0"
  MODULES ThickenLayeredCells::vtkThickenLayeredCellsFilters
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Filters/vtk.module")
