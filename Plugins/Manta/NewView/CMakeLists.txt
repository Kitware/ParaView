IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${VTK_INCLUDE_DIR}
    ${PARAVIEW_INCLUDE_DIRS}
    ${vtkManta_SOURCE_DIR}
    ${vtkManta_BINARY_DIR}
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
  QT4_WRAP_CPP(MOC_SRCS RegisterMantaView.h pqMantaView.h)

  ADD_PARAVIEW_VIEW_MODULE(
    VIEW_IFACE VIEW_IFACE_SRCS
    VIEW_TYPE pqMantaView
    VIEW_NAME Manta
    VIEW_XML_GROUP views
    VIEW_XML_NAME Manta
    )

ENDIF()

LINK_DIRECTORIES ( ${MANTA_BUILD_LIB} )

add_paraview_plugin(
  MantaView "2.0"
  REQUIRED_ON_SERVER
  REQUIRED_ON_CLIENT
  SERVER_MANAGER_SOURCES
    vtkPVMantaView.cxx
    vtkServerSideFactory.cxx
#   vtkSMMantaRepresentationProxy.cxx
    vtkSMMantaViewProxy.cxx
  SERVER_MANAGER_XML
    MantaView.xml
  GUI_INTERFACES
    ${IFACES} ${VIEW_IFACE}
  GUI_SOURCES
    ${IFACE_SRCS} ${VIEW_IFACE_SRCS}
    ${MOC_SRCS}
    RegisterMantaView.cxx pqMantaView.cxx
)

TARGET_LINK_LIBRARIES ( MantaView vtkMantaCS )
