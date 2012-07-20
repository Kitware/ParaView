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
    pqMultiSliceView.h)

  ADD_PARAVIEW_VIEW_MODULE(
    VIEW_IFACE VIEW_IFACE_SRCS
    VIEW_TYPE pqMultiSliceView
    VIEW_NAME MultiSlice
    VIEW_XML_GROUP views
    VIEW_XML_NAME MultiSlice)

  ADD_PARAVIEW_PLUGIN(MultiSliceView "1.0"
    SERVER_MANAGER_SOURCES
      vtkMultiSliceRepresentation.cxx
    SERVER_MANAGER_XML
      MultiSliceView.xml
    GUI_INTERFACES
      ${VIEW_IFACE}
    GUI_SOURCES
      ${VIEW_IFACE_SRCS}
      ${MOC_SRCS}
      vtkSlicesItem.cxx
      pqSliceAxisWidget.cxx
      pqMultiSliceView.cxx)

ENDIF(PARAVIEW_BUILD_QT_GUI)
