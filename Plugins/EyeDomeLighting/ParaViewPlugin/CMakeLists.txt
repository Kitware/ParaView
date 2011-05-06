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

add_paraview_view_module(
  ifaces iface_srcs
  VIEW_TYPE pqRenderView
  VIEW_XML_GROUP "views"
  VIEW_XML_NAME "RenderViewWithEDL"
  VIEW_NAME "Eye Dome Lighting")

add_paraview_plugin(
  EyeDomeLightingView "1.0"
  REQUIRED_ON_SERVER
  REQUIRED_ON_CLIENT
  SERVER_MANAGER_SOURCES
    vtkPVRenderViewWithEDL.cxx
  SERVER_MANAGER_XML
    RenderPassViews.xml
  GUI_INTERFACES
    ${ifaces}
  GUI_SOURCES
    ${iface_srcs}
    ${MOC_SRCS}
)

TARGET_LINK_LIBRARIES ( EyeDomeLightingView OffscreenShading)
