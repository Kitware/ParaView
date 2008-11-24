SET(CLIENT_GEO_VIEW_TILE_PATH "" CACHE PATH "Tile database path")
MARK_AS_ADVANCED(CLIENT_GEO_VIEW_TILE_PATH)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/ClientGeoViewConfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/ClientGeoViewConfig.h)

INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(
  MOC_SRCS 
  ClientGeoView.h 
  ClientGeoDisplay.h
  )

SET(
  UI_FORMS
  ClientGeoDisplay.ui
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
  VIEW_TYPE ClientGeoView 
  VIEW_NAME "Geospatial View"
  VIEW_XML_GROUP views 
  DISPLAY_XML ClientGeoDisplay 
  DISPLAY_PANEL ClientGeoDisplay
  )

ADD_PARAVIEW_PLUGIN(
  ClientGeoViewPlugin "1.0" 
  GUI_INTERFACES 
    ${IFACES} 
  SOURCES 
    ClientGeoView.cxx 
    ClientGeoDisplay.cxx 
    ${MOC_SRCS} 
    ${UI_BUILT_SOURCES}
    ${IFACE_SRCS} 
  SERVER_MANAGER_XML 
    ClientGeoView.xml
  )

TARGET_LINK_LIBRARIES(
  ClientGeoViewPlugin
  vtkGeovis
  vtkInfovis
  OverViewCore
  )

