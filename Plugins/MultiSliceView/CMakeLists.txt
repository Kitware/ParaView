#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIR}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

IF(PARAVIEW_BUILD_QT_GUI)
  INCLUDE(${QT_USE_FILE})
  INCLUDE_DIRECTORIES(${PARAVIEW_GUI_INCLUDE_DIRS})

  QT4_WRAP_CPP(MOC_SRCS
    pqActiveThreeSliceRenderViewOptions.h

    pqMultiSliceAxisWidget.h
    pqMultiSliceView.h

    pqThreeSliceView.h
    pqThreeSliceViewOptions.h

    vtkMultiSliceContextItem.h
  )

  QT4_WRAP_UI(UI_SRCS
    pqThreeSliceViewOptions.ui
  )

  ADD_PARAVIEW_VIEW_MODULE(
    VIEW1_IFACE VIEW1_IFACE_SRCS
    VIEW_TYPE pqMultiSliceView
    VIEW_NAME "Orthogonal Slice View"
    VIEW_XML_GROUP views
    VIEW_XML_NAME MultiSlice
  )

  ADD_PARAVIEW_VIEW_MODULE(
    VIEW2_IFACE VIEW2_IFACE_SRCS
    VIEW_TYPE pqThreeSliceView
    VIEW_NAME "ThreeSlice View"
    VIEW_XML_GROUP views
    VIEW_XML_NAME ThreeSlice
  )

  ADD_PARAVIEW_VIEW_OPTIONS(
    VIEW2_OPTIONS_IFACE VIEW2_OPTIONS_IFACE_SRCS
    VIEW_TYPE pqThreeSliceView
    ACTIVE_VIEW_OPTIONS pqActiveThreeSliceRenderViewOptions
  )

  ADD_PARAVIEW_PLUGIN(MultiSliceView "1.0"
    SERVER_MANAGER_SOURCES
      vtkCompositeMultiSliceRepresentation.cxx
      vtkMultiSliceRepresentation.cxx
      vtkThreeSliceFilter.cxx
      vtkSMMultiSliceViewProxy.cxx
    SERVER_MANAGER_XML
      SliceViews.xml
    GUI_INTERFACES
      ${VIEW1_IFACE}
      ${VIEW2_IFACE}
      ${VIEW2_OPTIONS_IFACE}
    GUI_SOURCES
      ${MOC_SRCS}
      ${UI_SRCS}
      ${VIEW1_IFACE_SRCS}
      ${VIEW2_IFACE_SRCS}
      ${VIEW2_OPTIONS_IFACE_SRCS}
      pqActiveThreeSliceRenderViewOptions.cxx

      pqMultiSliceAxisWidget.cxx
      pqMultiSliceView.cxx

      pqThreeSliceView.cxx
      pqThreeSliceViewOptions.cxx

      vtkMultiSliceContextItem.cxx
  )

ENDIF(PARAVIEW_BUILD_QT_GUI)


# Add testing if necessary
if (BUILD_TESTING)
  add_subdirectory(Testing)
endif (BUILD_TESTING)
