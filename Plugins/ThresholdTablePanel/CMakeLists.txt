INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(
  MOC_SRCS 
  ThresholdTablePanel.h
  )

SET(
  UI_FORMS
  ThresholdTablePanel.ui
  )

QT4_WRAP_UI(
  UI_BUILT_SOURCES
  ${UI_FORMS}
  )

SET(
  UI_RESOURCES
  ThresholdTablePanel.qrc
  )

QT4_ADD_RESOURCES(
  RCS_SOURCES
  ${UI_RESOURCES}
  )

SOURCE_GROUP("Resources" FILES
  ${UI_RESOURCES}
  ${UI_FORMS}
  )

SOURCE_GROUP("Generated" FILES
  ${MOC_SRCS}
  ${RCS_SOURCES}
  ${UI_BUILT_SOURCES}
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
  CLASS_NAME ThresholdTablePanel
  XML_NAME ThresholdTable
  XML_GROUP filters
  )

ADD_PARAVIEW_PLUGIN(
  ThresholdTablePanelPlugin "1.0"
  GUI_INTERFACES ${IFACES}
  SOURCES ${MOC_SRCS} ${RCS_SOURCES} ${UI_BUILT_SOURCES} ${IFACE_SRCS} ThresholdTablePanel.cxx
  )

TARGET_LINK_LIBRARIES(
  ThresholdTablePanelPlugin
  OverViewUtility
  pqWidgets
  )

