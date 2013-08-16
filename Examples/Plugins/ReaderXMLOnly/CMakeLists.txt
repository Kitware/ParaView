# This plugin can be loaded directly as the XML or compiled into a binary and
# then the binary can be loaded in ParaView.
cmake_minimum_required(VERSION 2.8)

if (NOT ParaView_BINARY_DIR)
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()

add_paraview_plugin(
  ReaderXMLOnly "1.0"
  SERVER_MANAGER_XML ReaderXMLOnly.xml
  REQUIRED_ON_SERVER)
