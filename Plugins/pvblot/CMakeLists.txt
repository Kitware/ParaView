# This plugin implements scripting that resembles commands from the SEACAS blot
# program.

SET(PYMODULES
  blotish.py
  pvblot.py
  )

SET(MOC_HEADERS
  pqBlotDialog.h
  pqBlotShell.h
  PVBlotPluginActions.h
  )

SET(UI_FILES
  pqBlotDialog.ui
  )

SET(CLIENT_SRCS
  pqBlotDialog.cxx
  pqBlotShell.cxx
  PVBlotPluginActions.cxx
  )

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

QT4_WRAP_CPP(MOC_SRCS ${MOC_HEADERS})
QT4_WRAP_UI(UI_SRCS ${UI_FILES})

ADD_PARAVIEW_ACTION_GROUP(IFACES IFACE_SRCS
  CLASS_NAME PVBlotPluginActions
  GROUP_NAME "MenuBar/Tools")

ADD_PARAVIEW_PLUGIN(pvblot "1.0"
  PYTHON_MODULES ${PYMODULES}
  GUI_INTERFACES ${IFACES}
  GUI_RESOURCES pvblot.qrc
  SOURCES ${MOC_SRCS} ${UI_SRCS} ${IFACE_SRCS} ${CLIENT_SRCS}
  )
