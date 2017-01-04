project(EyeDomeLighting)
cmake_minimum_required(VERSION 3.3)

if(NOT ParaView_SOURCE_DIR)
  find_package(ParaView)
  include("${PARAVIEW_USE_FILE}")
  include (ParaViewPlugins)

  # Load information about VTK/ParaView modules loaded/available.
  pv_setup_module_environment("EyeDomeLighting")
else()
  # we're building from within ParaView Source.
endif()

if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  # process all vtk-modules defined in this plugin. This generates the necessary
  # wrappings for all these modules as well.
  pv_process_modules()
endif()

# Process the ParaView plugin.
add_subdirectory(ParaViewPlugin)
