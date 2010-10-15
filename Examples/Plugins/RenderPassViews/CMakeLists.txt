# This plugin demonstrates how to add new render views to ParaView that use
# different render passes.
# It adds two views
# 1> Render View with Shadow Maps and
# 2> Render View with Edge detection

IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${VTK_INCLUDE_DIR}
    ${PARAVIEW_INCLUDE_DIRS}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

IF(PARAVIEW_BUILD_QT_GUI)
  QT4_WRAP_CPP(MOC_SRCS RegisterRenderPassViews.h)
ENDIF()

ADD_PARAVIEW_AUTO_START(
  IFACES
  IFACE_SRCS
  CLASS_NAME RegisterRenderPassViews
)

add_paraview_plugin(
  RenderPassViews "1.0"
  REQUIRED_ON_SERVER
  REQUIRED_ON_CLIENT
  SERVER_MANAGER_SOURCES
    vtkPVRenderViewWithSobel.cxx
    vtkPVRenderViewWithShadowMap.cxx
  SERVER_MANAGER_XML
    RenderPassViews.xml
  GUI_INTERFACES
    ${IFACES}
  GUI_SOURCES
    ${IFACE_SRCS}
    ${MOC_SRCS}
    RegisterRenderPassViews.cxx
)
