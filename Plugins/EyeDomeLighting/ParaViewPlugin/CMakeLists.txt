# This plugin demonstrates how to add new render views to ParaView that use
# different render passes.
# It adds one view
# 1> Render View with Eye Dome Lighting

IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${VTK_INCLUDE_DIR}
    ${PARAVIEW_INCLUDE_DIRS}
    ${EyeDomeLighting_SOURCE_DIR}/src
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${EyeDomeLighting_SOURCE_DIR}/src
  )
ENDIF (ParaView_SOURCE_DIR)
#message("${EDLPass_SOURCE_DIR}")
IF(PARAVIEW_BUILD_QT_GUI)
  INCLUDE(${QT_USE_FILE})
  include_directories(${QT_INCLUDES} ${PARAVIEW_GUI_INCLUDE_DIRS})
  QT4_WRAP_CPP(MOC_SRCS RegisterRenderPassViews.h)
ENDIF()

ADD_PARAVIEW_AUTO_START(
  IFACES
  IFACE_SRCS
  CLASS_NAME RegisterRenderPassViews
)

add_paraview_plugin(
  EyeDomeLightingView "1.0"
  REQUIRED_ON_SERVER
  REQUIRED_ON_CLIENT
  SERVER_MANAGER_SOURCES
    vtkPVRenderViewWithEDL.cxx
  SERVER_MANAGER_XML
    RenderPassViews.xml
  GUI_INTERFACES
    ${IFACES}
  GUI_SOURCES
    ${IFACE_SRCS}
    ${MOC_SRCS}
    RegisterRenderPassViews.cxx
)

TARGET_LINK_LIBRARIES ( EyeDomeLightingView OffscreenShading)
