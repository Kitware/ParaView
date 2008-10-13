INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(
  MOC_SRCS 
  SQLDatabaseGraphSourcePanel.h
  )

SET(
  UI_FORMS
  SQLDatabaseGraphSourcePanel.ui
  )

QT4_WRAP_UI(
  UI_BUILT_SOURCES
  ${UI_FORMS}
  )

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${OverViewUtility_BINARY_DIR}
  ${OverViewUtility_SOURCE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${ParaView_BINARY_DIR}/VTK/GUISupport/Qt
  ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
  ${QtWidgets_BINARY_DIR}
  ${QtWidgets_SOURCE_DIR}
  ${VTK_INCLUDE_DIR}
  ${pqComponents_BINARY_DIR}
  ${pqComponents_SOURCE_DIR}
  ${pqCore_BINARY_DIR}
  ${pqCore_SOURCE_DIR}
  )

ADD_PARAVIEW_OBJECT_PANEL(
  IFACES IFACE_SRCS
  CLASS_NAME SQLDatabaseGraphSourcePanel
  XML_NAME SQLDatabaseGraphSource
  XML_GROUP sources
  )

ADD_PARAVIEW_PLUGIN(
  SQLDatabaseGraphSourcePanelPlugin "1.0"
  GUI_INTERFACES ${IFACES}
  SOURCES ${MOC_SRCS} ${UI_BUILT_SOURCES} ${IFACE_SRCS} SQLDatabaseGraphSourcePanel.cxx
  )

TARGET_LINK_LIBRARIES(
  SQLDatabaseGraphSourcePanelPlugin
  OverViewUtility
  )

