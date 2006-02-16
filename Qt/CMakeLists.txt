CMAKE_MINIMUM_REQUIRED(VERSION 2.2)

FIND_PACKAGE(Qt4)
INCLUDE(${QT_USE_FILE})

IF(PARAQ_EMBED_PYTHON)
  SUBDIRS(
    Python
  )
ENDIF(PARAQ_EMBED_PYTHON)

SUBDIRS(
  Chart
  Components
  Testing
  Widgets
  Client)

INCLUDE_DIRECTORIES(
  ${ParaQ_BINARY_DIR}/ParaView
  ${ParaQ_BINARY_DIR}/ParaView/VTK
  ${ParaQ_BINARY_DIR}/ParaView/Servers/Common
  ${ParaQ_BINARY_DIR}/ParaView/Servers/Filters
  ${ParaQ_BINARY_DIR}/ParaView/Servers/ServerManager
  ${ParaQ_BINARY_DIR}/ParaView/Utilities/VTKClientServer
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Common
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Filtering
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/GenericFiltering
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/IO
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Rendering
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/GUISupport/Qt
  ${ParaQ_SOURCE_DIR}/ParaView/GUI/Client
  ${ParaQ_SOURCE_DIR}/ParaView/Servers/Common
  ${ParaQ_SOURCE_DIR}/ParaView/Servers/Filters
  ${ParaQ_SOURCE_DIR}/ParaView/Servers/ServerManager
  ${ParaQ_SOURCE_DIR}/ParaView/Utilities/VTKClientServer
)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.ctest.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.ctest" @ONLY IMMEDIATE)
