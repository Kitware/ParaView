CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/ClientGeoView2DConfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/ClientGeoView2DConfig.h)

INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(
  MOC_SRCS
  ClientGeoView2D.h
  ClientGeoDisplay2D.h
  )

SET(
  UI_FORMS
  ClientGeoDisplay2D.ui
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
  ${ParaView_SOURCE_DIR}/VTK/Geovis
  )

ADD_PARAVIEW_VIEW_MODULE(
  IFACES IFACE_SRCS
  VIEW_TYPE ClientGeoView2D
  VIEW_NAME "2D Geospatial View"
  VIEW_XML_GROUP views
  DISPLAY_XML ClientGeoDisplay2D
  DISPLAY_PANEL ClientGeoDisplay2D
  )

ADD_PARAVIEW_PLUGIN(
  ClientGeoView2DPlugin "1.0"
  GUI_INTERFACES
    ${IFACES}
  SOURCES
    ClientGeoView2D.cxx
    ClientGeoDisplay2D.cxx
    ${MOC_SRCS}
    ${UI_BUILT_SOURCES}
    ${IFACE_SRCS}
  SERVER_MANAGER_XML
    ClientGeoView2D.xml
  )

TARGET_LINK_LIBRARIES(
  ClientGeoView2DPlugin
  OverViewCore
  vtkGeovis
  )

