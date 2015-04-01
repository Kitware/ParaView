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
ELSE ()
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF ()

# Set a consistent MACOSX_RPATH default across all CMake versions.
# When CMake 2.8.12 is required, change this default to 1.
# When CMake 3.0.0 is required, remove this block (see CMP0042).
if(NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()

if (VTK_RENDERING_BACKEND STREQUAL "OpenGL")

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
endif()
