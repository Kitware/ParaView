paraview_add_plugin(OverrideXMLOnly
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  SERVER_MANAGER_XML OverrideXMLOnly.xml
  XML_DOCUMENTATION OFF)

option(BUILD_TESTING "Build Plugin Testing" OFF)
if (BUILD_TESTING AND BUILD_SHARED_LIBS)
  add_subdirectory(Testing)
endif()
