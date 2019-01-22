paraview_add_plugin(InSituExodus
  VERSION             "1.0"
  SERVER_MANAGER_XML  InSituExodus.xml)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
