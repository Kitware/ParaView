INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(MOC_SRCS
  ClientTreeAreaDisplay.h
  ClientTreeAreaView.h
  )

SET(UI_FORMS
  ClientTreeAreaDisplay.ui
  )

QT4_WRAP_UI(
  UI_BUILT_SOURCES
  ${UI_FORMS}
  )

SET(SOURCES
  ClientTreeAreaDisplay.cxx
  ClientTreeAreaView.cxx
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
  VIEW_TYPE ClientTreeAreaView
  VIEW_NAME "TreeArea View"
  VIEW_XML_GROUP views
  DISPLAY_XML ClientTreeAreaDisplay
  DISPLAY_PANEL ClientTreeAreaDisplay)

ADD_PARAVIEW_PLUGIN(
  ClientTreeAreaViewPlugin "1.0"
  GUI_INTERFACES ${IFACES} ${OPTIONS_IFACE}
  SOURCES ${SOURCES} ${MOC_SRCS} ${UI_BUILT_SOURCES} ${IFACE_SRCS} ${OPTIONS_IFACE_SRCS}
  SERVER_MANAGER_XML ClientTreeAreaView.xml
  )

TARGET_LINK_LIBRARIES(
  ClientTreeAreaViewPlugin
  OverViewCore
  )

