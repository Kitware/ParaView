paraview_add_plugin(PacMan
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION             "1.0"
  SERVER_MANAGER_XML  PacMan.xml)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
