# This plugin is simply for testing EULA support for plugins.
# This plugin is not built when BUILD_TESTING is OFF.

include_directories(${VTK_INCLUDE_DIRS})

add_paraview_plugin(EULATestPlugin "0.1"
  REQUIRED_ON_CLIENT
  SERVER_MANAGER_XML EULATestPlugin.xml
  EULA EULA.txt)
