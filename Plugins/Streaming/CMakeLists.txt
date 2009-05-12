INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
)

SET(STREAMING_SS_SOURCES
  vtkPiece.cxx 
  vtkPieceCacheFilter.cxx
  vtkPieceCacheExecutive.cxx
  vtkPieceList.cxx
  vtkPVSGeometryInformation.cxx
  vtkRawStridedReader.cxx
  vtkSMSImageDataParallelStrategy.cxx
  vtkSMStreamingOptionsProxy.cxx
  vtkSMStreamingOutputPort.cxx
  vtkSMStreamingRepresentation.cxx 
  vtkSMStreamingSerialStrategy.cxx 
  vtkSMStreamingViewHelper.cxx
  vtkSMStreamingViewProxy.cxx
  vtkSMSUniformGridParallelStrategy.cxx
  vtkSMSUnstructuredDataParallelStrategy.cxx
  vtkSMSUnstructuredGridParallelStrategy.cxx
  vtkStreamingFactory.cxx
  vtkStreamingOptions.cxx
  vtkStreamingUpdateSuppressor.cxx
  vtkVisibilityPrioritizer.cxx
)

# arguments for the server side pieces of the plugin
SET(SERVER_ARGS
  SERVER_MANAGER_XML StreamingServerWrapping.xml 
  SERVER_MANAGER_SOURCES ${STREAMING_SS_SOURCES}
  )
  
IF(PARAVIEW_BUILD_QT_GUI)
  FIND_PACKAGE(Qt4)
  IF(QT4_FOUND)
    
    INCLUDE(${QT_USE_FILE})
    
    INCLUDE_DIRECTORIES(
      ${VTK_INCLUDE_DIR}
      ${PARAVIEW_INCLUDE_DIRS}
      ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
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
    
    # invoke macro to create the streaming view.
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
    
    # invoke macro to create the options dialog
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
    
    # arguments to create the client side pieces of the plugin
    SET(CLIENT_ARGS
      GUI_INTERFACES 
      ${IFACES} ${OPTIONS_IFACE} ${PANEL_IFACE}
      GUI_SOURCES 
      ${STREAMING_CS_SOURCES}
      ${MOC_SRCS} 
      ${IFACE_SRCS} ${OPTIONS_IFACE_SRCS} ${PANEL_IFACE_SRCS}
      ${UI_BUILT_SOURCES})
    
  ENDIF(QT4_FOUND)
  
ENDIF(PARAVIEW_BUILD_QT_GUI)

#combine the server and client sides together into a single plugin
#paraview is smart enough to only load the pieces that make sense
ADD_PARAVIEW_PLUGIN ( StreamingPlugin "1.0" 
  ${SERVER_ARGS}
  ${CLIENT_ARGS}
  )

#if python is enabled, wrap the plugin so that it can be used from within python code
IF (PARAVIEW_ENABLE_PYTHON)
  SET(DONT_WRAP_LIST
    vtkStreamingUpdateSuppressor.cxx
    vtkPVSGeometryInformation.cxx
    vtkSMStreamingViewHelper.cxx
    )
  WRAP_PLUGIN_FOR_PYTHON(StreamingPlugin "${STREAMING_SS_SOURCES}" "${DONT_WRAP_LIST}")
ENDIF (PARAVIEW_ENABLE_PYTHON)

#configure regression tests
ADD_SUBDIRECTORY(Testing)

