paraview_add_plugin(AnalyzeNIfTIReaderWriter
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES AnalyzeNIfTIIO::NIfTIIO
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/NIfTIIO/vtk.module")

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
