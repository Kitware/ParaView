include_directories(
  ${VTK_INCLUDE_DIRS}
  ${MantaView_SOURCE_DIR}/VTK
  ${MantaView_BINARY_DIR}/VTK
)

IF(PARAVIEW_BUILD_QT_GUI)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    )
  SET (MantaView_SRCS
    MantaView.h)
  IF (PARAVIEW_QT_VERSION VERSION_GREATER "4")
    INCLUDE (ParaViewQt5)
    QT5_WRAP_CPP(MOC_SRCS
      ${MantaView_SRCS})
  ELSE ()
    INCLUDE (${QT_USE_FILE})
    QT4_WRAP_CPP(MOC_SRCS
      ${MantaView_SRCS})
  ENDIF ()
  add_pqproxy(
    VIEW_IFACE VIEW_IFACE_SRCS
    TYPE MantaView
    XML_GROUP views
    XML_NAME Manta)
ENDIF()

LINK_DIRECTORIES ( ${MANTA_BUILD_LIB} )

add_paraview_plugin(
  MantaView "2.0"
  REQUIRED_ON_SERVER
  REQUIRED_ON_CLIENT
  SERVER_MANAGER_SOURCES
    vtkPVMantaView.cxx
    vtkSMMantaViewProxy.cxx
  SERVER_MANAGER_XML
    MantaView.xml
  GUI_INTERFACES
    ${VIEW_IFACE} ${OPTIONS_IFACE} ${DISPLAY_IFACE}
  GUI_SOURCES
    ${VIEW_IFACE_SRCS} ${OPTIONS_IFACE_SRCS} ${DISPLAY_IFACE_SRCS} ${UI_SRCS}
    ${MOC_SRCS}
    MantaView.cxx
)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif()
