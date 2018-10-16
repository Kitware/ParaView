project(PanoramicProjection)
cmake_minimum_required(VERSION 3.3)

if(NOT ParaView_SOURCE_DIR)
  find_package(ParaView)
  include("${PARAVIEW_USE_FILE}")
  include (ParaViewPlugins)

  # Load information about VTK/ParaView modules loaded/available.
  pv_setup_module_environment("PanoramicProjection")
endif()

include_directories(
  ${VTK_INCLUDE_DIRS}
  ${PARAVIEW_INCLUDE_DIRS}
  ${PARAVIEW_GUI_INCLUDE_DIRS}
  )

add_paraview_plugin(
  PanoramicProjectionView "1.0"
  DESCRIPTION "Add a panoramic projection 3D view"
  REQUIRED_ON_SERVER
  REQUIRED_ON_CLIENT
  SERVER_MANAGER_SOURCES
    vtkPVPanoramicProjectionView.cxx
  SERVER_MANAGER_XML
    PanoramicProjectionView.xml
)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif()
