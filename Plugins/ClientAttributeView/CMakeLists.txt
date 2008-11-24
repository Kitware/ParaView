INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(
  MOC_SRCS
  ClientAttributeView.h
  ClientAttributeDisplay.h
  )

SET(UI_FORMS
  ClientAttributeDisplay.ui
  ClientAttributeView.ui
  )

QT4_WRAP_UI(
  UI_BUILT_SOURCES
  ${UI_FORMS}
  )

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${OverViewCore_BINARY_DIR}
  ${OverViewCore_SOURCE_DIR}
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

ADD_PARAVIEW_VIEW_MODULE(
  IFACES IFACE_SRCS
  VIEW_TYPE ClientAttributeView
  VIEW_NAME "Attribute View"
  VIEW_XML_GROUP views
  DISPLAY_XML ClientAttributeDisplay
  DISPLAY_PANEL ClientAttributeDisplay
  )

ADD_PARAVIEW_PLUGIN(
  ClientAttributeViewPlugin "1.0"
  GUI_INTERFACES ${IFACES}
  SOURCES ClientAttributeView.cxx ClientAttributeDisplay.cxx ${MOC_SRCS} ${UI_BUILT_SOURCES} ${IFACE_SRCS}
  SERVER_MANAGER_XML ClientAttributeView.xml
  )

TARGET_LINK_LIBRARIES(
  ClientAttributeViewPlugin
  OverViewCore
  )

