list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

paraview_add_plugin(AdiosReaderPixie
  VERSION "1.4"
  MODULES AdiosReaderPixie::Pixie
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Pixie/vtk.module")
