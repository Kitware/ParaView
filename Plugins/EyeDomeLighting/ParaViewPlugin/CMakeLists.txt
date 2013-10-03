include_directories(${VTK_INCLUDE_DIRS}
  ${EyeDomeLighting_SOURCE_DIR}/EyeDomeLighting
  ${EyeDomeLighting_BINARY_DIR}/EyeDomeLighting)
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

# Add testing if necessary
if (BUILD_TESTING)
  add_subdirectory(Testing)
endif (BUILD_TESTING)
