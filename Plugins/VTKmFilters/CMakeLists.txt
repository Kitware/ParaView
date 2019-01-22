paraview_add_plugin(VTKmFilters
  REQUIRED_ON_SERVER
  VERSION             "0.1"
  SERVER_MANAGER_XML  VTKmSM.xml)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
