include_directories(${VTK_INCLUDE_DIRS})
add_paraview_plugin(ThickenLayeredCells "1.0"
  SERVER_MANAGER_XML ThickenLayeredCells.xml
  SERVER_MANAGER_SOURCES vtkThickenLayeredCells.cxx)
