# This plugin can be loaded directly as the XML or compiled into a binary and
# then the binary can be loaded in ParaView.
cmake_minimum_required(VERSION 3.3)

if (NOT ParaView_BINARY_DIR)
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()

# Set a consistent MACOSX_RPATH default across all CMake versions.
# When CMake 2.8.12 is required, change this default to 1.
# When CMake 3.0.0 is required, remove this block (see CMP0042).
if(NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()

add_paraview_plugin(
  ReaderXMLOnly "1.0"
  SERVER_MANAGER_XML ReaderXMLOnly.xml
  REQUIRED_ON_SERVER)
