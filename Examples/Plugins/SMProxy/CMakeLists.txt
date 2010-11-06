
# create a paraview plugin containing custom classes that work on the client
# side in the server manager

# in this example, a custom bounds domain is created so that 
# the low and high point of the elevation filter can be initialized
# in the z-direction

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

IF(NOT ParaView_BINARY_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF(NOT ParaView_BINARY_DIR)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SET(SM_SOURCES vtkSMMyElevationProxy.cxx vtkSMMyBoundsDomain.cxx)

# we need server manager wrappings for SM_SOURCES
# put instantiators in GUI_SOURCES because they are client side
# and we need the plugin to be loaded on the client side
ADD_PARAVIEW_PLUGIN(SMMyProxy "1.0"
  SERVER_MANAGER_XML MyProxy.xml
  SERVER_MANAGER_SOURCES ${SM_SOURCES})

