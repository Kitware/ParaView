set(interfaces)
set(sources
  pqMyApplicationStarter.cxx
  pqMyApplicationStarter.h)

paraview_plugin_add_auto_start(
  CLASS_NAME pqMyApplicationStarter
  STARTUP onStartup
  SHUTDOWN onShutdown
  INTERFACES autostart_interface
  SOURCES autostart_sources)
list(APPEND interfaces
  ${autostart_interface})
list(APPEND sources
  ${autostart_sources})

paraview_add_plugin(Autostart
  REQUIRED_ON_CLIENT
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources})

target_link_libraries(Autostart
  PRIVATE
    ParaView::pqApplicationComponents)

option(BUILD_TESTING "Build Plugin Testing" OFF)
if (BUILD_TESTING AND BUILD_SHARED_LIBS)
  add_subdirectory(Testing)
endif()
