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
  pv_qt_wrap_cpp(MOC_SRCS ${MantaView_SRCS} pqLightsWidget.h)
  add_paraview_property_group_widget(IFACES IFACE_SRCS
    TYPE "extra_lights"
    CLASS_NAME pqLightsWidget)
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
    ${IFACES} ${VIEW_IFACE}
  GUI_SOURCES
    ${IFACE_SRCS} ${VIEW_IFACE_SRCS} ${UI_SRCS}
    ${MOC_SRCS}
    pqLightsWidget.cxx
    MantaView.cxx
)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif()
