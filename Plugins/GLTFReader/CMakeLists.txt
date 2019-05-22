paraview_add_plugin(GLTFReader
  VERSION             "1.0"
  SERVER_MANAGER_XML  GLTFReader.xml)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
