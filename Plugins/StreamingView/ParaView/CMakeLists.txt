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
    VIEW_NAME IteratingView
    VIEW_XML_GROUP views
    VIEW_XML_NAME IteratingView
    )

  ADD_PARAVIEW_VIEW_MODULE( VIEW2_IFACE VIEW2_IFACE_SRCS
    VIEW_TYPE PrioritizingView
    VIEW_NAME PrioritizingView
    VIEW_XML_GROUP views
    VIEW_XML_NAME PrioritizingView
    )

  ADD_PARAVIEW_VIEW_MODULE( VIEW3_IFACE VIEW3_IFACE_SRCS
    VIEW_TYPE RefiningView
    VIEW_NAME RefiningView
    VIEW_XML_GROUP views
    VIEW_XML_NAME RefiningView
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
    vtkSMStreamingRepresentationProxy.cxx
    vtkSMStreamingViewProxy.cxx
    vtkStreamLibraryWrapper.cxx
  SERVER_MANAGER_XML
    StreamingView.xml
  GUI_RESOURCE_FILES
    StreamingReaders.xml
  GUI_INTERFACES
    ${VIEW1_IFACE}
    ${VIEW2_IFACE}
    ${VIEW3_IFACE}
    ${INSPECTOR_IFACE1}
  GUI_SOURCES
    ${MOC_SRCS}
    ${UI_BUILT_SOURCES}
    ${VIEW1_IFACE_SRCS}
    ${VIEW2_IFACE_SRCS}
    ${VIEW3_IFACE_SRCS}
    ${INSPECTOR_IFACE1_SRCS}
    StreamingView.cxx
    IteratingView.cxx
    PrioritizingView.cxx
    RefiningView.cxx
    pqStreamingControls.cxx
)

TARGET_LINK_LIBRARIES ( StreamingView vtkStreamingCS )
