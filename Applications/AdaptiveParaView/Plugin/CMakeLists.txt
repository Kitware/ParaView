INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
)

SET(ADAPTIVE_SS_SOURCES
  vtkPiece.cxx 
  vtkPieceCacheFilter.cxx
  vtkPieceCacheExecutive.cxx
  vtkPieceList.cxx
  vtkPVSGeometryInformation.cxx
  vtkRawStridedReader1.cxx
  vtkGridSampler1.cxx
  vtkGridSampler2.cxx
  vtkACosmoReader.cxx
  vtkMetaInfoDatabase.cxx
#  vtkSMSImageDataParallelStrategy.cxx
  vtkSMAdaptiveOutlineRepresentation.cxx 
  vtkSMAdaptiveOutputPort.cxx
  vtkSMAdaptiveRepresentation.cxx 
  vtkSMAdaptiveSerialStrategy.cxx 
  vtkSMAdaptiveOptionsProxy.cxx
  vtkSMAdaptiveViewHelper.cxx
  vtkSMAdaptiveViewProxy.cxx 
#  vtkSMSUniformGridParallelStrategy.cxx
#  vtkSMSUnstructuredDataParallelStrategy.cxx
#  vtkSMSUnstructuredGridParallelStrategy.cxx
  vtkAdaptiveFactory.cxx
  vtkAdaptiveOptions.cxx
  vtkAdaptiveUpdateSuppressor.cxx
  vtkVisibilityPrioritizer.cxx
)

IF(NOT WIN32)
  SET(ADAPTIVE_SS_SOURCES ${ADAPTIVE_SS_SOURCES} vtkRawStridedReader2.cxx)
ENDIF(NOT WIN32)

# arguments for the server side pieces of the plugin
SET(SERVER_ARGS
  SERVER_MANAGER_XML AdaptiveWrapping.xml 
  SERVER_MANAGER_SOURCES ${ADAPTIVE_SS_SOURCES}
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
      pqAdaptiveRenderView.h
      pqGlobalAdaptiveViewOptions.h
      pqAdaptiveDisplayDecorator.h
      pqAdaptiveControls.h
      )
    
    QT4_WRAP_UI(UI_BUILT_SOURCES
      pqGlobalAdaptiveViewOptions.ui
      pqAdaptiveDisplayDecorator.ui
      )
    
    SET(ADAPTIVE_CS_SOURCES
      pqAdaptiveRenderView.cxx
      pqGlobalAdaptiveViewOptions.cxx
      pqAdaptiveDisplayDecorator.cxx
      pqAdaptiveControls.cxx
      )

    # invoke macro to create the adaptive view.
    ADD_PARAVIEW_VIEW_MODULE(
      VIEW_IFACE
      VIEW_IFACE_SRCS 
      VIEW_TYPE pqAdaptiveRenderView
      VIEW_NAME AdaptiveView
      VIEW_XML_GROUP views
      VIEW_XML_NAME Adaptive
      DISPLAY_XML
      DISLAY_PANEL
      )
    
    # invoke macro to create the options dialog
    ADD_PARAVIEW_VIEW_OPTIONS(
      OPTIONS_IFACE
      OPTIONS_IFACE_SRCS
      VIEW_TYPE pqAdaptiveRenderView
      GLOBAL_VIEW_OPTIONS pqGlobalAdaptiveViewOptions
      )

    # invoke macro that adds the show piece bounds and refinement locking checkboxes
    # to the display tab
    ADD_PARAVIEW_DISPLAY_PANEL_DECORATOR(
      PANEL_IFACE PANEL_IFACE_SRCS 
      CLASS_NAME pqAdaptiveDisplayDecorator
      PANEL_TYPES pqDisplayProxyEditor)

    # invoke macro that adds the refinement and display controls panel
    ADD_PARAVIEW_DOCK_WINDOW(
      INSPECTOR_IFACE INSPECTOR_IFACE_SRCS 
      CLASS_NAME pqAdaptiveControls 
      DOCK_AREA Left)

    # arguments to create the client side pieces of the plugin
    SET(CLIENT_ARGS
      GUI_INTERFACES 
      ${VIEW_IFACE} ${OPTIONS_IFACE} ${PANEL_IFACE} ${INSPECTOR_IFACE}
      GUI_SOURCES 
      ${ADAPTIVE_CS_SOURCES}
      ${MOC_SRCS} 
      ${VIEW_IFACE_SRCS} ${OPTIONS_IFACE_SRCS} ${PANEL_IFACE_SRCS} ${INSPECTOR_IFACE_SRCS}
      ${UI_BUILT_SOURCES})
        
  ENDIF(QT4_FOUND)

ENDIF(PARAVIEW_BUILD_QT_GUI)

#combine the server and client sides together into a single plugin
#paraview is smart enough to only load the pieces that make sense
ADD_PARAVIEW_PLUGIN ( AdaptivePlugin "1.0" 
  ${SERVER_ARGS}
  ${CLIENT_ARGS}
  )
  
#if python is enabled, wrap the plugin so that it can be used from within python code
IF (PARAVIEW_ENABLE_PYTHON)
  SET(DONT_WRAP_LIST
    vtkAdaptiveUpdateSuppressor.cxx
    vtkPVSGeometryInformation.cxx
    vtkSMAdaptiveViewHelper.cxx
    )
  WRAP_PLUGIN_FOR_PYTHON(AdaptivePlugin "${ADAPTIVE_SS_SOURCES}" "${DONT_WRAP_LIST}")
ENDIF (PARAVIEW_ENABLE_PYTHON)

#configure regression tests
ADD_SUBDIRECTORY(Testing)
