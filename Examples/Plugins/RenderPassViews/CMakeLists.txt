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

add_paraview_plugin(
  RenderPassViews "1.0"
  REQUIRED_ON_SERVER
  REQUIRED_ON_CLIENT
  SERVER_MANAGER_SOURCES
    vtkPVRenderViewWithSobel.cxx
    vtkPVRenderViewWithShadowMap.cxx
  SERVER_MANAGER_XML
    RenderPassViews.xml
)
