FIND_PACKAGE(Qt4)
IF(QT4_FOUND)

INCLUDE(${QT_USE_FILE})

INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
  ${ParaView_SOURCE_DIR}/Plugins/StreamingPlugins/Server
  ${pqComponents_SOURCE_DIR}
  ${pqComponents_BINARY_DIR}
  ${pqCore_SOURCE_DIR}
  ${pqCore_BINARY_DIR}
  ${QtWidgets_SOURCE_DIR}
  ${QtWidgets_BINARY_DIR}
  ${QtChart_SOURCE_DIR}
  ${QtChart_BINARY_DIR}
  ${QtTesting_SOURCE_DIR}
  ${QtTesting_BINARY_DIR}
  ${QtPython_SOURCE_DIR}
  ${QtPython_BINARY_DIR}
  ${ParaView_SOURCE_DIR}/Plugins/Streaming/Server
)

# moc the Qt based .h files
QT4_WRAP_CPP(MOC_SRCS
  pqStreamingRenderView.h
  pqGlobalStreamingViewOptions.h
  pqStreamingDisplayDecorator.h
)

QT4_WRAP_UI(UI_BUILT_SOURCES
  pqGlobalStreamingViewOptions.ui
  pqStreamingDisplayDecorator.ui
)

SET(STREAMING_CS_SOURCES
  pqStreamingRenderView.cxx
  pqGlobalStreamingViewOptions.cxx
  pqStreamingDisplayDecorator.cxx
)

# invoke macro to create sources for streaming view.
ADD_PARAVIEW_VIEW_MODULE(
                         IFACES  
                         IFACE_SRCS 
                         VIEW_TYPE pqStreamingRenderView
                         VIEW_NAME StreamingView
                         VIEW_XML_GROUP views
                         VIEW_XML_NAME Streaming
                         DISPLAY_XML
                         DISLAY_PANEL
                         )

# invoke macro to create sources for options dialog
ADD_PARAVIEW_VIEW_OPTIONS(
                         OPTIONS_IFACE
                         OPTIONS_IFACE_SRCS
                         VIEW_TYPE pqStreamingRenderView
                         GLOBAL_VIEW_OPTIONS pqGlobalStreamingViewOptions
                         )

# invoke macro that adds the show piece bounds checkbox to the display tab
ADD_PARAVIEW_DISPLAY_PANEL_DECORATOR(
  PANEL_IFACE PANEL_IFACE_SRCS 
  CLASS_NAME pqStreamingDisplayDecorator
  PANEL_TYPES pqDisplayProxyEditor)

# finally create the plugin that puts the above together into a lib that 
# paraview can load
ADD_PARAVIEW_PLUGIN(StreamingCSPlugin "1.0" 
                    GUI_INTERFACES 
                    ${IFACES} ${OPTIONS_IFACE} ${PANEL_IFACE}
                    GUI_SOURCES 
                    ${STREAMING_CS_SOURCES}
                    ${MOC_SRCS} 
                    ${IFACE_SRCS} ${OPTIONS_IFACE_SRCS} ${PANEL_IFACE_SRCS}
                    ${UI_BUILT_SOURCES})


TARGET_LINK_LIBRARIES(StreamingCSPlugin StreamingSSPlugin)

ENDIF(QT4_FOUND)
