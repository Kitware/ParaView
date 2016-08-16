
# create a paraview plugin containing custom classes that work on the client
# side in the server manager

# in this example, a custom bounds domain is created so that 
# the low and high point of the elevation filter can be initialized
# in the z-direction

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

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SET(SM_SOURCES vtkSMMyElevationProxy.cxx vtkSMMyBoundsDomain.cxx)

# we need server manager wrappings for SM_SOURCES
# put instantiators in GUI_SOURCES because they are client side
# and we need the plugin to be loaded on the client side
ADD_PARAVIEW_PLUGIN(SMMyProxy "1.0"
  SERVER_MANAGER_XML MyProxy.xml
  SERVER_MANAGER_SOURCES ${SM_SOURCES})

