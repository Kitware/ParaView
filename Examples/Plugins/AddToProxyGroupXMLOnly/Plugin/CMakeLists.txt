paraview_add_plugin(AddToProxyGroupXMLOnly
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  SERVER_MANAGER_XML AddToProxyGroupXMLOnly.xml)

option(BUILD_TESTING "Build Plugin Testing" OFF)
if (BUILD_TESTING AND BUILD_SHARED_LIBS)
  add_subdirectory(Testing)
endif()
