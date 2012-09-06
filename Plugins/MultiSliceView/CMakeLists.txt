#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIRS}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

IF(PARAVIEW_BUILD_QT_GUI)
  INCLUDE(${QT_USE_FILE})
  INCLUDE_DIRECTORIES(${PARAVIEW_GUI_INCLUDE_DIRS})

  QT4_WRAP_CPP(MOC_SRCS
    pqMultiSliceAxisWidget.h
    pqMultiSliceView.h

    pqQuadView.h
    pqQuadViewOptions.h
    pqActiveQuadViewOptions.h
  )

  QT4_WRAP_UI(UI_SRCS
    pqQuadViewOptions.ui
  )

  ADD_PARAVIEW_VIEW_MODULE(
    VIEW1_IFACE VIEW1_IFACE_SRCS
    VIEW_TYPE pqMultiSliceView
    VIEW_NAME "Orthogonal Slice View"
    VIEW_XML_GROUP views
    VIEW_XML_NAME MultiSlice
  )

  add_paraview_view_module(
    VIEW2_IFACE VIEW2_IFACE_SRCS
    VIEW_TYPE pqQuadView
    VIEW_NAME "Quad View"
    VIEW_XML_GROUP "views"
    VIEW_XML_NAME "QuadView")

  ADD_PARAVIEW_VIEW_OPTIONS(
    VIEW2_OPTIONS_IFACE VIEW2_OPTIONS_IFACE_SRCS
    VIEW_TYPE pqQuadView
    ACTIVE_VIEW_OPTIONS pqActiveQuadViewOptions
  )

  ADD_PARAVIEW_PLUGIN(SliceViews "1.0"

    SERVER_MANAGER_SOURCES
      vtkCompositeSliceRepresentation.cxx
      vtkPVMultiSliceView.cxx
      vtkPVQuadRenderView.cxx
      vtkSMMultiSliceViewProxy.cxx
      vtkSMQuadViewProxy.cxx
      vtkSliceFriendGeometryRepresentation.cxx
      vtkThreeSliceFilter.cxx

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
      pqActiveQuadViewOptions.cxx

      pqMultiSliceAxisWidget.cxx
      pqMultiSliceView.cxx

      pqQuadView.cxx
      pqQuadViewOptions.cxx
      pqActiveQuadViewOptions.cxx

      vtkMultiSliceContextItem.cxx
  )

ENDIF(PARAVIEW_BUILD_QT_GUI)

# Add testing if necessary
if (BUILD_TESTING)
  add_subdirectory(Testing)
endif (BUILD_TESTING)
