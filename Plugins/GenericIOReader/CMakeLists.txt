paraview_add_plugin(GenericIOReader
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES GenericIOReader::vtkGenericIOReader
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Readers/vtk.module")
