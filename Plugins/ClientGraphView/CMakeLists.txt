INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(
  MOC_SRCS
  ClientGraphView.h
  ClientGraphDisplay.h
  )

SET(UI_FORMS
  ClientGraphDisplay.ui
  )

QT4_WRAP_UI(UI_BUILT_SOURCES
  ${UI_FORMS}
  )

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${OverViewUtility_SOURCE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${ParaView_BINARY_DIR}/VTK/GUISupport/Qt
  ${ParaView_SOURCE_DIR}/Plugins/PrismPlugins/Server
  ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
  ${QtWidgets_BINARY_DIR}
  ${QtWidgets_SOURCE_DIR}
  ${VTK_INCLUDE_DIR}
  ${pqComponents_BINARY_DIR}
  ${pqComponents_SOURCE_DIR}
  ${pqCore_BINARY_DIR}
  ${pqCore_SOURCE_DIR}
  )

ADD_PARAVIEW_VIEW_MODULE(
  IFACES IFACE_SRCS
  VIEW_TYPE ClientGraphView
  VIEW_NAME "Graph View"
  VIEW_XML_GROUP views
  DISPLAY_XML ClientGraphDisplay
  DISPLAY_PANEL ClientGraphDisplay
  )

ADD_PARAVIEW_PLUGIN(
  ClientGraphViewPlugin "1.0"
  GUI_INTERFACES ${IFACES}
  SOURCES ClientGraphView.cxx ClientGraphDisplay.cxx ${MOC_SRCS} ${UI_BUILT_SOURCES} ${IFACE_SRCS}
  SERVER_MANAGER_XML ClientGraphView.xml
  )

TARGET_LINK_LIBRARIES(
  ClientGraphViewPlugin
  OverViewUtility
  )

