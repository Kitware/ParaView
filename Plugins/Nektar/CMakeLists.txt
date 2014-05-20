cmake_minimum_required(VERSION 2.8.8)
project(Nektar)

if(NOT ParaView_SOURCE_DIR)
  find_package(ParaView)
  include("${PARAVIEW_USE_FILE}")
  include (ParaViewPlugins)

  # Load information about VTK/ParaView modules loaded/available.
  pv_setup_module_environment("Nektar")
else()
  # we're building from within ParaView Source.
endif()

# process all vtk-modules defined in this plugin. This generates the necessary
# wrappings for all these modules as well.
pv_process_modules()

# Process the ParaView plugin.
add_subdirectory(ParaViewPlugin)
