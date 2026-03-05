set(interfaces)
set(sources
  pqAbortTesting.cxx
  pqAbortTesting.h)

paraview_plugin_add_auto_start(
  CLASS_NAME pqAbortTesting
  STARTUP onStartup
  SHUTDOWN onShutdown
  INTERFACES autostart_interface
  SOURCES autostart_sources)
list(APPEND interfaces
  ${autostart_interface})
list(APPEND sources
  ${autostart_sources})

paraview_add_plugin(AbortTesting
  REQUIRED_ON_CLIENT
  VERSION "1.0"
  EULA    Disclaimer.txt
  UI_INTERFACES ${interfaces}
  SOURCES ${sources})

add_subdirectory(Testing)
