if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  include_directories(${VTK_INCLUDE_DIRS}
    ${EyeDomeLighting_SOURCE_DIR}/EyeDomeLighting
    ${EyeDomeLighting_BINARY_DIR}/EyeDomeLighting)
else()
  include_directories(
    ${VTK_INCLUDE_DIRS}
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    )
endif()

add_paraview_plugin(
  EyeDomeLightingView "1.0"
  REQUIRED_ON_SERVER
  REQUIRED_ON_CLIENT
  SERVER_MANAGER_SOURCES
    vtkPVRenderViewWithEDL.cxx
  SERVER_MANAGER_XML
    RenderPassViews.xml
)

if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  target_link_libraries(EyeDomeLightingView
    LINK_PRIVATE vtkEyeDomeLighting)
endif()

# Add testing if necessary
if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
