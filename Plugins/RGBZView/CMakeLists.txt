project(RGBZView)
cmake_minimum_required(VERSION 2.8.8)

if(NOT ParaView_SOURCE_DIR)
  find_package(ParaView)
  include("${PARAVIEW_USE_FILE}")
  include(ParaViewPlugins)

  # Load information about VTK/ParaView modules loaded/available.
  pv_setup_module_environment("RGBZView")
else()
  # we're building from within ParaView Source.
endif()

# process all vtk-modules defined in this plugin. This generates the necessary
# wrappings for all these modules as well.
pv_process_modules()

# Add headers for VTK classes
include_directories(${VTK_INCLUDE_DIRS})
add_paraview_plugin(
  RGBZView "1.0"
  REQUIRED_ON_SERVER
  REQUIRED_ON_CLIENT
  SERVER_MANAGER_SOURCES
    vtkPVRenderViewForAssembly.cxx
  SERVER_MANAGER_XML
    RGBZViews.xml
)
