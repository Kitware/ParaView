IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${VTK_INCLUDE_DIR}
    ${PARAVIEW_INCLUDE_DIRS}
    ${vtkStreaming_SOURCE_DIR}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

IF(PARAVIEW_BUILD_QT_GUI)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    )
  INCLUDE(${QT_USE_FILE})

  QT4_WRAP_CPP(MOC_SRCS
    IteratingView.h
    PrioritizingView.h
    RefiningView.h
    StreamingView.h
    pqStreamingControls.h
    )

  QT4_WRAP_UI(UI_BUILT_SOURCES
    pqStreamingControls.ui
    )

  ADD_PARAVIEW_VIEW_MODULE( VIEW1_IFACE VIEW1_IFACE_SRCS
    VIEW_TYPE IteratingView
    VIEW_NAME "Iterating View"
    VIEW_XML_GROUP views
    VIEW_XML_NAME IteratingView
    )

  ADD_PARAVIEW_VIEW_OPTIONS(
    VIEW1_OPTIONS_IFACE
    VIEW1_OPTIONS_SRCS
    VIEW_TYPE IteratingView
    ACTIVE_VIEW_OPTIONS pqActiveRenderViewOptions
  )

  ADD_PARAVIEW_VIEW_MODULE( VIEW2_IFACE VIEW2_IFACE_SRCS
    VIEW_TYPE PrioritizingView
    VIEW_NAME "Prioritizing View"
    VIEW_XML_GROUP views
    VIEW_XML_NAME PrioritizingView
    )

  ADD_PARAVIEW_VIEW_OPTIONS(
    VIEW2_OPTIONS_IFACE
    VIEW2_OPTIONS_SRCS
    VIEW_TYPE PrioritizingView
    ACTIVE_VIEW_OPTIONS pqActiveRenderViewOptions
  )

  ADD_PARAVIEW_VIEW_MODULE( VIEW3_IFACE VIEW3_IFACE_SRCS
    VIEW_TYPE RefiningView
    VIEW_NAME "Refining View"
    VIEW_XML_GROUP views
    VIEW_XML_NAME RefiningView
    )

  ADD_PARAVIEW_VIEW_OPTIONS(
    VIEW3_OPTIONS_IFACE
    VIEW3_OPTIONS_SRCS
    VIEW_TYPE RefiningView
    ACTIVE_VIEW_OPTIONS pqActiveRenderViewOptions
  )

  # invoke macro that adds the refinement view control panel
  ADD_PARAVIEW_DOCK_WINDOW(
    INSPECTOR_IFACE1 INSPECTOR_IFACE1_SRCS
    CLASS_NAME pqStreamingControls
    DOCK_AREA Left)

ENDIF()

add_paraview_plugin(
  StreamingView "2.0"
  REQUIRED_ON_SERVER
  REQUIRED_ON_CLIENT
  SERVER_MANAGER_SOURCES
    vtkPVStreamingRepresentation.cxx
    vtkPVStreamingView.cxx
    vtkPVStreamingParallelHelper.cxx
    vtkSIStreamingRepresentationProxy.cxx
    vtkSMStreamingViewProxy.cxx
  SERVER_MANAGER_XML
    StreamingView.xml
  GUI_RESOURCE_FILES
    StreamingReaders.xml
  GUI_INTERFACES
    ${VIEW1_IFACE}
    ${VIEW1_OPTIONS_IFACE}
    ${VIEW2_IFACE}
    ${VIEW2_OPTIONS_IFACE}
    ${VIEW3_IFACE}
    ${VIEW3_OPTIONS_IFACE}
    ${INSPECTOR_IFACE1}
  GUI_SOURCES
    ${MOC_SRCS}
    ${UI_BUILT_SOURCES}
    ${VIEW1_IFACE_SRCS}
    ${VIEW1_OPTIONS_SRCS}
    ${VIEW2_IFACE_SRCS}
    ${VIEW2_OPTIONS_SRCS}
    ${VIEW3_IFACE_SRCS}
    ${VIEW3_OPTIONS_SRCS}
    ${INSPECTOR_IFACE1_SRCS}
    StreamingView.cxx
    IteratingView.cxx
    PrioritizingView.cxx
    RefiningView.cxx
    pqStreamingControls.cxx
  CS_KITS vtkStreaming
)

TARGET_LINK_LIBRARIES ( StreamingView vtkStreamingCS )
