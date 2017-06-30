include_directories(
  ${VTK_INCLUDE_DIRS}
  ${PARAVIEW_INCLUDE_DIRS}
  ${PARAVIEW_GUI_INCLUDE_DIRS}
  )

add_paraview_plugin(
  EyeDomeLightingView "1.0"
  REQUIRED_ON_SERVER
  REQUIRED_ON_CLIENT
  SERVER_MANAGER_SOURCES
    vtkPVRenderViewWithEDL.cxx
  SERVER_MANAGER_XML
    RenderPassViews.xml
)

# Add testing if necessary
if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
