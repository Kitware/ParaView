set(interfaces)
set(sources
  pqTranslationsTesting.cxx
  pqTranslationsTesting.h)

paraview_plugin_add_auto_start(
  CLASS_NAME pqTranslationsTesting
  STARTUP onStartup
  SHUTDOWN onShutdown
  INTERFACES autostart_interface
  SOURCES autostart_sources)
list(APPEND interfaces
  ${autostart_interface})
list(APPEND sources
  ${autostart_sources})

paraview_add_plugin(TranslationsTesting
  REQUIRED_ON_CLIENT
  VERSION "1.0"
  EULA    Disclaimer.txt
  UI_INTERFACES ${interfaces}
  SOURCES ${sources})

add_subdirectory(Testing)
