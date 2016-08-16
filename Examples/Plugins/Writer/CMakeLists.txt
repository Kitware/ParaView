
# create a plugin that adds a reader to the ParaView GUI
# it is added in the file dialog when doing opens/saves.

cmake_minimum_required(VERSION 3.3)

IF(NOT ParaView_BINARY_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF()

# Set a consistent MACOSX_RPATH default across all CMake versions.
# When CMake 2.8.12 is required, change this default to 1.
# When CMake 3.0.0 is required, remove this block (see CMP0042).
if(NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()

ADD_PARAVIEW_PLUGIN(MyTiffWriter "1.0"
  SERVER_MANAGER_XML writers.xml
  REQUIRED_ON_SERVER)
