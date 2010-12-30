
# create a paraview plugin containing server manager xml and the server
# manager classes to build
# this plugin can be loaded on the server side

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

IF(NOT ParaView_BINARY_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF(NOT ParaView_BINARY_DIR)

ADD_PARAVIEW_PLUGIN(SMParametricSource "1.0"
  SERVER_MANAGER_XML ParametricSource.xml)

