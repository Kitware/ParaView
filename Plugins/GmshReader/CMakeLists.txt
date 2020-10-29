if(NOT ${PARAVIEW_BUILD_LEGACY_REMOVE})
  if(NOT ${PARAVIEW_BUILD_LEGACY_SILENT})
    message(DEPRECATION "The GmshReader plugin is deprecated in the favor of the GmshIO plugin.")
  endif()

  list(INSERT CMAKE_MODULE_PATH 0
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

  paraview_add_plugin(GmshReader
    REQUIRED_ON_SERVER
    VERSION "1.0"
    MODULES GmshReader::vtkGmshReader
    MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Reader/vtk.module")

  if (BUILD_TESTING)
    add_subdirectory(Testing)
  endif ()
endif()
