list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

paraview_add_plugin(VDFReaderPlugin
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES VDFReaderPlugin::vtkVaporReaders
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Readers/vtk.module")
