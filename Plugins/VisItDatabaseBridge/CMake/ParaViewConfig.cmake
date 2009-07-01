# +---------------------------------------------------------------------------+
# |                                                                           |
# |                          Locate ParaView build                            |
# |                                                                           |
# +---------------------------------------------------------------------------+
#ParaView3

set(ParaView_DIR 
  ${ParaView_BINARY_DIR}
  CACHE FILEPATH
  "Path to ParaView build.")

INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(
    ${VTK_INCLUDE_DIR}
    ${PARAVIEW_INCLUDE_DIRS}
    ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
    ${pqCore_SOURCE_DIR}
    ${pqCore_BINARY_DIR}
    ${pqComponents_SOURCE_DIR}
    ${pqComponents_BINARY_DIR}
    ${QtWidgets_SOURCE_DIR}
    ${QtWidgets_BINARY_DIR}
    )
