paraview_add_plugin(PacMan
  VERSION             "1.0"
  SERVER_MANAGER_XML  PacMan.xml)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
