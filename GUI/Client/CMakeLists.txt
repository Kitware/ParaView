PROJECT(PQCLIENT)

# Set the variable for include directories for moc.
SET (MOC_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/Widgets
  #${CMAKE_CURRENT_BINARY_DIR}/Widgets
  )

SUBDIRS(
  Widgets)

# Everything under Client uses VTK/ParaView ServerManager.
INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
  ${QtWidgets_SOURCE_DIR}
  ${QtWidgets_BINARY_DIR}
  ${QtChart_SOURCE_DIR}
  ${QtChart_BINARY_DIR}
  ${QtTesting_SOURCE_DIR}
  ${QtTesting_BINARY_DIR}
)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.ctest.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.ctest" @ONLY IMMEDIATE)
