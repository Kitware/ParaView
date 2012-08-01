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
    vtkSlicesItem.h
    pqSliceAxisWidget.h
    pqMultiSliceView.h
    pqThreeSliceView.h
  )

  ADD_PARAVIEW_VIEW_MODULE(
    VIEW1_IFACE VIEW1_IFACE_SRCS
    VIEW_TYPE pqMultiSliceView
    VIEW_NAME "MultiSlice"
    VIEW_XML_GROUP views
    VIEW_XML_NAME MultiSlice)

  ADD_PARAVIEW_VIEW_MODULE(
    VIEW2_IFACE VIEW2_IFACE_SRCS
    VIEW_TYPE pqThreeSliceView
    VIEW_NAME "ThreeSliceView"
    VIEW_XML_GROUP views
    VIEW_XML_NAME ThreeSlice)

  ADD_PARAVIEW_PLUGIN(MultiSliceView "1.0"
    SERVER_MANAGER_SOURCES
      vtkMultiSliceRepresentation.cxx
    SERVER_MANAGER_XML
      MultiSliceView.xml
    GUI_INTERFACES
      ${VIEW1_IFACE}
      ${VIEW2_IFACE}
    GUI_SOURCES
      ${VIEW1_IFACE_SRCS}
      ${VIEW2_IFACE_SRCS}
      ${MOC_SRCS}
      vtkSlicesItem.cxx
      pqSliceAxisWidget.cxx
      pqMultiSliceView.cxx
      pqThreeSliceView.cxx
  )

ENDIF(PARAVIEW_BUILD_QT_GUI)
