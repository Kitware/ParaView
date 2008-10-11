INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(MOC_SRCS
  ClientHierarchyDisplay.h
  ClientHierarchyView.h
  ClientHierarchyViewActiveOptions.h
  ClientHierarchyViewOptions.h
  )

SET(UI_FORMS
  ClientHierarchyDisplay.ui
  ClientHierarchyView.ui
  ClientHierarchyViewOptions.ui
  )

QT4_WRAP_UI(
  UI_BUILT_SOURCES
  ${UI_FORMS}
  )

SET(SOURCES
  ClientHierarchyDisplay.cxx
  ClientHierarchyView.cxx
  ClientHierarchyViewActiveOptions.cxx
  ClientHierarchyViewOptions.cxx  
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

ADD_PARAVIEW_VIEW_MODULE(
  IFACES IFACE_SRCS
  VIEW_TYPE ClientHierarchyView
  VIEW_NAME "Hierarchy View"
  VIEW_XML_GROUP views
  DISPLAY_XML ClientHierarchyDisplay
  DISPLAY_PANEL ClientHierarchyDisplay)

ADD_PARAVIEW_VIEW_OPTIONS(
  OPTIONS_IFACE OPTIONS_IFACE_SRCS
  VIEW_TYPE ClientHierarchyView
  ACTIVE_VIEW_OPTIONS ClientHierarchyViewActiveOptions)

ADD_PARAVIEW_PLUGIN(
  ClientHierarchyViewPlugin "1.0"
  GUI_INTERFACES ${IFACES} ${OPTIONS_IFACE}
  SOURCES ${SOURCES} ${MOC_SRCS} ${UI_BUILT_SOURCES} ${IFACE_SRCS} ${OPTIONS_IFACE_SRCS}
  SERVER_MANAGER_XML ClientHierarchyView.xml
  SERVER_MANAGER_SOURCES ${VTK_SOURCE_DIR}/Infovis/vtkPassThrough.h
  )

TARGET_LINK_LIBRARIES(
  ClientHierarchyViewPlugin
  OverViewUtility
  )

