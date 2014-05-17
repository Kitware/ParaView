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
  INCLUDE_DIRECTORIES(${PARAVIEW_GUI_INCLUDE_DIRS})
  IF (PARAVIEW_QT_VERSION VERSION_GREATER "4")
    SET (Qt5_FIND_COMPONENTS Widgets)
    INCLUDE (ParaViewQt5)

    QT5_WRAP_CPP(MOC_SRCS
      pqQuadView.h
      )
  ELSE ()
    INCLUDE (${QT_USE_FILE})

    QT4_WRAP_CPP(MOC_SRCS
      pqQuadView.h
      )
  ENDIF ()

  add_pqproxy(VIEW1_IFACE VIEW1_IFACE_SRCS
    TYPE pqQuadView
    XML_GROUP "views"
    XML_NAME "QuadView")

  ADD_PARAVIEW_PLUGIN(QuadView "1.0"

    SERVER_MANAGER_SOURCES
      vtkPVQuadRenderView.cxx
      vtkPVQuadViewInformation.cxx
      vtkQuadRepresentation.cxx
      vtkSMQuadViewProxy.cxx

    SERVER_MANAGER_XML
      SliceViews.xml

    GUI_INTERFACES
      ${VIEW1_IFACE}

    GUI_SOURCES
      ${MOC_SRCS}
      ${UI_SRCS}
      ${VIEW1_IFACE_SRCS}

      pqQuadView.cxx
  )

ENDIF(PARAVIEW_BUILD_QT_GUI)

# Add testing if necessary
if (BUILD_TESTING)
  add_subdirectory(Testing)
endif (BUILD_TESTING)
