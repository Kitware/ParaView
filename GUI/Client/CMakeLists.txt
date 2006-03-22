PROJECT(PQCLIENT)

IF(PARAQ_EMBED_PYTHON)
  SUBDIRS(Python)
ENDIF(PARAQ_EMBED_PYTHON)

SUBDIRS(
  Chart
  Components
  Testing
  Widgets
  Client)

# Everything under Client uses VTK/ParaView ServerManager.
INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${ParaQ_SOURCE_DIR}/VTK/GUISupport/Qt
  ${Widgets_SOURCE_DIR}
  ${Widgets_BINARY_DIR}
)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.ctest.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.ctest" @ONLY IMMEDIATE)
