include_directories(${VTK_INCLUDE_DIRS}
  ${EyeDomeLighting_SOURCE_DIR}/EyeDomeLighting
  ${EyeDomeLighting_BINARY_DIR}/EyeDomeLighting)

add_paraview_plugin(
  EyeDomeLightingView "1.0"
  REQUIRED_ON_SERVER
  REQUIRED_ON_CLIENT
  SERVER_MANAGER_SOURCES
    vtkPVRenderViewWithEDL.cxx
  SERVER_MANAGER_XML
    RenderPassViews.xml
)
target_link_libraries(EyeDomeLightingView
  LINK_PRIVATE vtkEyeDomeLighting)

# Add testing if necessary
if (BUILD_TESTING)
  add_subdirectory(Testing)
endif (BUILD_TESTING)
