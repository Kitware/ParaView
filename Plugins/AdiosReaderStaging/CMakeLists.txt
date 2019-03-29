list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

paraview_add_plugin(AdiosReaderStaging
  VERSION "1.4"
  MODULES AdiosReaderStaging::Staging
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Staging/vtk.module")
